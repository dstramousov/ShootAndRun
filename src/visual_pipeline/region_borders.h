#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_REGION_BORDERS_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_REGION_BORDERS_H_

/**
 * @file src/visual_pipeline/region_borders.h
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains public
 * declarations for region_borders.h.
 */

#include <string>
#include <vector>

#include "level/level_data.h"
#include "level/terrain_type.h"
#include "visual_pipeline/semantic_masks.h"
#include "visual_pipeline/terrain_regions.h"

namespace sar::visual_pipeline {

/**
 * @brief Stores border tile data shared between runtime systems.
 */
struct BorderTile {
  int tile_index = 0;  ///< Tile index value carried by this data structure.
  int x = 0;  ///< Tile, screen, or world coordinate for x.
  int y = 0;  ///< Tile, screen, or world coordinate for y.
  int region_id = 0;  ///< Stable identifier for region ID.
  TerrainType region_type = TerrainType::kUnknown;  ///< Semantic type for region.
  TerrainType north = TerrainType::kUnknown;  ///< North value carried by this data structure.
  TerrainType east = TerrainType::kUnknown;  ///< East value carried by this data structure.
  TerrainType south = TerrainType::kUnknown;  ///< South value carried by this data structure.
  TerrainType west = TerrainType::kUnknown;  ///< West value carried by this data structure.
  bool north_differs = false;  ///< North differs value carried by this data structure.
  bool east_differs = false;  ///< East differs value carried by this data structure.
  bool south_differs = false;  ///< South differs value carried by this data structure.
  bool west_differs = false;  ///< West differs value carried by this data structure.
  bool touches_map_edge = false;  ///< Touches map edge value carried by this data structure.
  int differing_neighbor_count = 0;  ///< Count of differing neighbor count entries or events.

  /**
   * @brief Returns true when the tile has an adjacent outside corner.
   *
   * @return Corner classification flag.
   */
  bool IsCorner() const;

  /**
   * @brief Returns a readable dump of the border tile.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

/**
 * @brief Stores region border info data shared between runtime systems.
 */
struct RegionBorderInfo {
  int region_id = 0;  ///< Stable identifier for region ID.
  TerrainType type = TerrainType::kUnknown;  ///< Semantic type loaded from source data or configuration.
  int total_border_tiles = 0;  ///< Total border tiles value carried by this data structure.
  int edge_tile_count = 0;  ///< Count of edge tile count entries or events.
  int corner_tile_count = 0;  ///< Count of corner tile count entries or events.
  int thin_tile_count = 0;  ///< Count of thin tile count entries or events.
  int complex_tile_count = 0;  ///< Count of complex tile count entries or events.
  int map_edge_tile_count = 0;  ///< Count of map edge tile count entries or events.
  int neighbor_open_ground = 0;  ///< Neighbor open ground value carried by this data structure.
  int neighbor_forest = 0;  ///< Neighbor forest value carried by this data structure.
  int neighbor_road = 0;  ///< Neighbor road value carried by this data structure.
  int neighbor_swamp = 0;  ///< Neighbor swamp value carried by this data structure.
  int neighbor_water = 0;  ///< Neighbor water value carried by this data structure.
  int neighbor_ruins = 0;  ///< Neighbor ruins value carried by this data structure.
  int neighbor_wall = 0;  ///< Neighbor wall value carried by this data structure.
  int neighbor_unknown = 0;  ///< Neighbor unknown value carried by this data structure.
  int neighbor_outside_map = 0;  ///< Neighbor outside map value carried by this data structure.
  std::vector<BorderTile> tiles;  ///< Tiles value carried by this data structure.

  /**
   * @brief Returns a readable dump of region border counters.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

/**
 * @brief Stores region border summary data shared between runtime systems.
 */
struct RegionBorderSummary {
  int region_count = 0;  ///< Count of region count entries or events.
  int border_tile_count = 0;  ///< Count of border tile count entries or events.
  int edge_tile_count = 0;  ///< Count of edge tile count entries or events.
  int corner_tile_count = 0;  ///< Count of corner tile count entries or events.
  int thin_tile_count = 0;  ///< Count of thin tile count entries or events.
  int complex_tile_count = 0;  ///< Count of complex tile count entries or events.
  int map_edge_tile_count = 0;  ///< Count of map edge tile count entries or events.
  int neighbor_open_ground = 0;  ///< Neighbor open ground value carried by this data structure.
  int neighbor_forest = 0;  ///< Neighbor forest value carried by this data structure.
  int neighbor_road = 0;  ///< Neighbor road value carried by this data structure.
  int neighbor_swamp = 0;  ///< Neighbor swamp value carried by this data structure.
  int neighbor_water = 0;  ///< Neighbor water value carried by this data structure.
  int neighbor_ruins = 0;  ///< Neighbor ruins value carried by this data structure.
  int neighbor_wall = 0;  ///< Neighbor wall value carried by this data structure.
  int neighbor_unknown = 0;  ///< Neighbor unknown value carried by this data structure.
  int neighbor_outside_map = 0;  ///< Neighbor outside map value carried by this data structure.

  /**
   * @brief Returns a readable summary of region border counters.
   *
   * @return String representation for logs and debug overlays.
   */
  std::string Dump() const;
};

/**
 * @brief Stores region borders data shared between runtime systems.
 */
struct RegionBorders {
  LevelSize size;  ///< Size value carried by this data structure.
  std::vector<RegionBorderInfo> regions;  ///< Regions value carried by this data structure.
  RegionBorderSummary summary;  ///< Summary value carried by this data structure.

  /**
   * @brief Returns true when border data matches the terrain regions.
   *
   * @return Validation flag.
   */
  bool IsValid() const;

  /**
   * @brief Returns a readable dump of all border counters.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

/**
 * @brief Classifies terrain region borders for later smoothing passes.
 *
 * The classifier inspects every region border tile, records neighboring
 * terrain types, and marks edge/corner/thin/complex tile shapes. The output is
 * data-only and does not depend on raylib or rendering APIs.
 *
 * @param masks Semantic masks produced from the raw level.
 * @param terrain_regions Connected terrain regions built from the masks.
 * @param error Error text populated when classification fails.
 * @return Classified region borders. Returned data is invalid on failure.
 */
RegionBorders ClassifyRegionBorders(const SemanticMasks& masks,
                                     const TerrainRegions& terrain_regions,
                                     std::string* error);

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_REGION_BORDERS_H_
