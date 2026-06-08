/**
 * @file src/visual_pipeline/region_borders.cpp
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains
 * implementation for region_borders.cpp.
 */

#include "visual_pipeline/region_borders.h"

#include <cstddef>
#include <string>
#include <utility>

namespace sar::visual_pipeline {
namespace {

/**
 * @brief Converts tile coordinates to a linear grid index.
 */
int ToIndex(int x, int y, int width) {
  return y * width + x;
}

/**
 * @brief Executes the tile x operation.
 */
int TileX(int index, int width) {
  return index % width;
}

/**
 * @brief Executes the tile y operation.
 */
int TileY(int index, int width) {
  return index / width;
}

/**
 * @brief Checks whether tile coordinates are inside the level bounds.
 */
bool IsInside(int x, int y, const LevelSize& size) {
  return x >= 0 && y >= 0 && x < size.width && y < size.height;
}

/**
 * @brief Executes the mask value at operation.
 */
bool MaskValueAt(const std::vector<std::uint8_t>& mask, int index) {
  return mask[static_cast<std::size_t>(index)] != 0;
}

/**
 * @brief Executes the terrain at index operation.
 */
TerrainType TerrainAtIndex(const SemanticMasks& masks, int index) {
  if (MaskValueAt(masks.open_ground, index)) {
    return TerrainType::kOpenGround;
  }
  if (MaskValueAt(masks.forest, index)) {
    return TerrainType::kForest;
  }
  if (MaskValueAt(masks.road, index)) {
    return TerrainType::kRoad;
  }
  if (MaskValueAt(masks.swamp, index)) {
    return TerrainType::kSwamp;
  }
  if (MaskValueAt(masks.water, index)) {
    return TerrainType::kWater;
  }
  if (MaskValueAt(masks.ruins, index)) {
    return TerrainType::kRuins;
  }
  if (MaskValueAt(masks.wall, index)) {
    return TerrainType::kWall;
  }
  return TerrainType::kUnknown;
}

/**
 * @brief Executes the neighbor terrain or unknown operation.
 */
TerrainType NeighborTerrainOrUnknown(const SemanticMasks& masks, int x, int y) {
  if (!IsInside(x, y, masks.size)) {
    return TerrainType::kUnknown;
  }

  return TerrainAtIndex(masks, ToIndex(x, y, masks.size.width));
}

/**
 * @brief Increments terrain counter.
 */
void IncrementTerrainCounter(TerrainType type, RegionBorderInfo* info) {
  if (info == nullptr) {
    return;
  }

  switch (type) {
    case TerrainType::kOpenGround:
      ++info->neighbor_open_ground;
      break;
    case TerrainType::kForest:
      ++info->neighbor_forest;
      break;
    case TerrainType::kRoad:
      ++info->neighbor_road;
      break;
    case TerrainType::kSwamp:
      ++info->neighbor_swamp;
      break;
    case TerrainType::kWater:
      ++info->neighbor_water;
      break;
    case TerrainType::kRuins:
      ++info->neighbor_ruins;
      break;
    case TerrainType::kWall:
      ++info->neighbor_wall;
      break;
    case TerrainType::kUnknown:
      ++info->neighbor_unknown;
      break;
  }
}

/**
 * @brief Adds neighbor.
 */
void AddNeighbor(bool differs, bool outside_map, TerrainType terrain,
                 BorderTile* tile, RegionBorderInfo* info) {
  if (tile == nullptr || info == nullptr || !differs) {
    return;
  }

  ++tile->differing_neighbor_count;
  if (outside_map) {
    tile->touches_map_edge = true;
    ++info->neighbor_outside_map;
    return;
  }

  IncrementTerrainCounter(terrain, info);
}

/**
 * @brief Classifies tile shape.
 */
void ClassifyTileShape(const BorderTile& tile, RegionBorderInfo* info) {
  if (info == nullptr) {
    return;
  }

  if (tile.differing_neighbor_count <= 1) {
    ++info->edge_tile_count;
    return;
  }

  if (tile.differing_neighbor_count == 2) {
    if (tile.IsCorner()) {
      ++info->corner_tile_count;
    } else {
      ++info->thin_tile_count;
    }
    return;
  }

  ++info->complex_tile_count;
}

/**
 * @brief Returns classify border tile.
 */
BorderTile ClassifyBorderTile(const SemanticMasks& masks,
                              const TerrainRegion& region, int tile_index,
                              RegionBorderInfo* info) {
  BorderTile tile;
  tile.tile_index = tile_index;
  tile.x = TileX(tile_index, masks.size.width);
  tile.y = TileY(tile_index, masks.size.width);
  tile.region_id = region.id;
  tile.region_type = region.type;

  const int north_y = tile.y - 1;
  const int east_x = tile.x + 1;
  const int south_y = tile.y + 1;
  const int west_x = tile.x - 1;

  const bool north_inside = IsInside(tile.x, north_y, masks.size);
  const bool east_inside = IsInside(east_x, tile.y, masks.size);
  const bool south_inside = IsInside(tile.x, south_y, masks.size);
  const bool west_inside = IsInside(west_x, tile.y, masks.size);

  tile.north = NeighborTerrainOrUnknown(masks, tile.x, north_y);
  tile.east = NeighborTerrainOrUnknown(masks, east_x, tile.y);
  tile.south = NeighborTerrainOrUnknown(masks, tile.x, south_y);
  tile.west = NeighborTerrainOrUnknown(masks, west_x, tile.y);

  tile.north_differs = !north_inside || tile.north != region.type;
  tile.east_differs = !east_inside || tile.east != region.type;
  tile.south_differs = !south_inside || tile.south != region.type;
  tile.west_differs = !west_inside || tile.west != region.type;

  AddNeighbor(tile.north_differs, !north_inside, tile.north, &tile, info);
  AddNeighbor(tile.east_differs, !east_inside, tile.east, &tile, info);
  AddNeighbor(tile.south_differs, !south_inside, tile.south, &tile, info);
  AddNeighbor(tile.west_differs, !west_inside, tile.west, &tile, info);

  if (tile.touches_map_edge) {
    ++info->map_edge_tile_count;
  }

  ClassifyTileShape(tile, info);
  return tile;
}

/**
 * @brief Classifies region border.
 */
RegionBorderInfo ClassifyRegionBorder(const SemanticMasks& masks,
                                       const TerrainRegion& region) {
  RegionBorderInfo info;
  info.region_id = region.id;
  info.type = region.type;
  info.total_border_tiles = region.border_tile_count;
  info.tiles.reserve(region.border_tile_indices.size());

  for (int tile_index : region.border_tile_indices) {
    info.tiles.push_back(ClassifyBorderTile(masks, region, tile_index,
                                            &info));
  }

  return info;
}

/**
 * @brief Updates summary for region for the current frame.
 */
void UpdateSummaryForRegion(const RegionBorderInfo& info,
                            RegionBorderSummary* summary) {
  if (summary == nullptr) {
    return;
  }

  ++summary->region_count;
  summary->border_tile_count += info.total_border_tiles;
  summary->edge_tile_count += info.edge_tile_count;
  summary->corner_tile_count += info.corner_tile_count;
  summary->thin_tile_count += info.thin_tile_count;
  summary->complex_tile_count += info.complex_tile_count;
  summary->map_edge_tile_count += info.map_edge_tile_count;
  summary->neighbor_open_ground += info.neighbor_open_ground;
  summary->neighbor_forest += info.neighbor_forest;
  summary->neighbor_road += info.neighbor_road;
  summary->neighbor_swamp += info.neighbor_swamp;
  summary->neighbor_water += info.neighbor_water;
  summary->neighbor_ruins += info.neighbor_ruins;
  summary->neighbor_wall += info.neighbor_wall;
  summary->neighbor_unknown += info.neighbor_unknown;
  summary->neighbor_outside_map += info.neighbor_outside_map;
}

}  // namespace

/**
 * @brief Checks whether corner is true.
 */
bool BorderTile::IsCorner() const {
  return (north_differs && east_differs) || (east_differs && south_differs) ||
         (south_differs && west_differs) || (west_differs && north_differs);
}

/**
 * @brief Builds a readable diagnostic dump for dump.
 */
std::string BorderTile::Dump() const {
  return "BorderTile { region_id: " + std::to_string(region_id) +
         ", type: " + std::string(TerrainTypeToString(region_type)) +
         ", x: " + std::to_string(x) + ", y: " + std::to_string(y) +
         ", differs: " + std::to_string(differing_neighbor_count) +
         ", map_edge: " + std::string(touches_map_edge ? "true" : "false") +
         " }";
}

/**
 * @brief Builds a readable diagnostic dump for dump.
 */
std::string RegionBorderInfo::Dump() const {
  return "RegionBorderInfo { region_id: " + std::to_string(region_id) +
         ", type: " + std::string(TerrainTypeToString(type)) +
         ", border_tiles: " + std::to_string(total_border_tiles) +
         ", edge: " + std::to_string(edge_tile_count) +
         ", corner: " + std::to_string(corner_tile_count) +
         ", thin: " + std::to_string(thin_tile_count) +
         ", complex: " + std::to_string(complex_tile_count) +
         ", map_edge: " + std::to_string(map_edge_tile_count) + " }";
}

/**
 * @brief Builds a readable diagnostic dump for dump.
 */
std::string RegionBorderSummary::Dump() const {
  return "RegionBorderSummary { regions: " + std::to_string(region_count) +
         ", border_tiles: " + std::to_string(border_tile_count) +
         ", edge: " + std::to_string(edge_tile_count) +
         ", corner: " + std::to_string(corner_tile_count) +
         ", thin: " + std::to_string(thin_tile_count) +
         ", complex: " + std::to_string(complex_tile_count) +
         ", map_edge: " + std::to_string(map_edge_tile_count) +
         ", neighbor_open: " + std::to_string(neighbor_open_ground) +
         ", neighbor_forest: " + std::to_string(neighbor_forest) +
         ", neighbor_road: " + std::to_string(neighbor_road) +
         ", neighbor_swamp: " + std::to_string(neighbor_swamp) +
         ", neighbor_water: " + std::to_string(neighbor_water) +
         ", neighbor_ruins: " + std::to_string(neighbor_ruins) +
         ", neighbor_wall: " + std::to_string(neighbor_wall) +
         ", neighbor_unknown: " + std::to_string(neighbor_unknown) +
         ", neighbor_outside: " + std::to_string(neighbor_outside_map) +
         " }";
}

/**
 * @brief Checks whether valid is true.
 */
bool RegionBorders::IsValid() const {
  if (size.width <= 0 || size.height <= 0) {
    return false;
  }

  return summary.region_count == static_cast<int>(regions.size());
}

/**
 * @brief Builds a readable diagnostic dump for dump.
 */
std::string RegionBorders::Dump() const {
  return "RegionBorders { size: " + std::to_string(size.width) + "x" +
         std::to_string(size.height) + ", " + summary.Dump() + " }";
}

/**
 * @brief Classifies region borders.
 */
RegionBorders ClassifyRegionBorders(const SemanticMasks& masks,
                                     const TerrainRegions& terrain_regions,
                                     std::string* error) {
  RegionBorders borders;
  borders.size = masks.size;

  if (!masks.IsValid()) {
    if (error != nullptr) {
      *error = "semantic masks are invalid for border classification";
    }
    return borders;
  }

  if (!terrain_regions.IsValid()) {
    if (error != nullptr) {
      *error = "terrain regions are invalid for border classification";
    }
    return borders;
  }

  if (terrain_regions.size.width != masks.size.width ||
      terrain_regions.size.height != masks.size.height) {
    if (error != nullptr) {
      *error = "terrain region size does not match semantic masks";
    }
    return borders;
  }

  borders.regions.reserve(terrain_regions.regions.size());
  for (const TerrainRegion& region : terrain_regions.regions) {
    RegionBorderInfo info = ClassifyRegionBorder(masks, region);
    UpdateSummaryForRegion(info, &borders.summary);
    borders.regions.push_back(std::move(info));
  }

  return borders;
}

}  // namespace sar::visual_pipeline
