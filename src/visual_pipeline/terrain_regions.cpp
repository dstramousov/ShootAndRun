#include "visual_pipeline/terrain_regions.h"

#include <algorithm>
#include <cstddef>
#include <queue>
#include <string>
#include <utility>
#include <vector>

namespace sar::visual_pipeline {
namespace {

constexpr int kTinyRegionMaxArea = 3;

struct MaskDescriptor {
  TerrainType type;
  const std::vector<std::uint8_t>* mask;
};

int ToIndex(int x, int y, int width) {
  return y * width + x;
}

int TileX(int index, int width) {
  return index % width;
}

int TileY(int index, int width) {
  return index / width;
}

bool IsInside(int x, int y, const LevelSize& size) {
  return x >= 0 && y >= 0 && x < size.width && y < size.height;
}

bool IsMaskSet(const std::vector<std::uint8_t>& mask, int x, int y,
               const LevelSize& size) {
  if (!IsInside(x, y, size)) {
    return false;
  }

  const int index = ToIndex(x, y, size.width);
  return mask[static_cast<std::size_t>(index)] != 0;
}

bool IsBorderTile(const std::vector<std::uint8_t>& mask, int x, int y,
                  const LevelSize& size) {
  constexpr int kNeighborCount = 4;
  const int dx[kNeighborCount] = {1, -1, 0, 0};
  const int dy[kNeighborCount] = {0, 0, 1, -1};

  for (int neighbor = 0; neighbor < kNeighborCount; ++neighbor) {
    const int nx = x + dx[neighbor];
    const int ny = y + dy[neighbor];
    if (!IsMaskSet(mask, nx, ny, size)) {
      return true;
    }
  }

  return false;
}

void UpdateSummaryForRegion(const TerrainRegion& region,
                            TerrainRegionSummary* summary) {
  if (summary == nullptr) {
    return;
  }

  ++summary->total_regions;
  summary->largest_region_area = std::max(summary->largest_region_area,
                                          region.area);
  if (region.area <= kTinyRegionMaxArea) {
    ++summary->tiny_regions;
  }

  switch (region.type) {
    case TerrainType::kOpenGround:
      ++summary->open_ground_regions;
      summary->largest_open_ground_area = std::max(
          summary->largest_open_ground_area, region.area);
      break;
    case TerrainType::kForest:
      ++summary->forest_regions;
      summary->largest_forest_area = std::max(summary->largest_forest_area,
                                              region.area);
      break;
    case TerrainType::kRoad:
      ++summary->road_regions;
      break;
    case TerrainType::kSwamp:
      ++summary->swamp_regions;
      break;
    case TerrainType::kWater:
      ++summary->water_regions;
      break;
    case TerrainType::kRuins:
      ++summary->ruins_regions;
      break;
    case TerrainType::kWall:
      ++summary->wall_regions;
      break;
    case TerrainType::kUnknown:
      ++summary->unknown_regions;
      break;
  }
}

TerrainRegion BuildRegionFromSeed(const std::vector<std::uint8_t>& mask,
                                   TerrainType type, int seed_index,
                                   int region_id, const LevelSize& size,
                                   std::vector<std::uint8_t>* visited) {
  TerrainRegion region;
  region.id = region_id;
  region.type = type;
  region.min_x = size.width;
  region.min_y = size.height;
  region.max_x = 0;
  region.max_y = 0;

  std::queue<int> pending;
  pending.push(seed_index);
  (*visited)[static_cast<std::size_t>(seed_index)] = 1;

  while (!pending.empty()) {
    const int index = pending.front();
    pending.pop();

    const int x = TileX(index, size.width);
    const int y = TileY(index, size.width);
    region.tile_indices.push_back(index);
    ++region.area;
    region.min_x = std::min(region.min_x, x);
    region.min_y = std::min(region.min_y, y);
    region.max_x = std::max(region.max_x, x);
    region.max_y = std::max(region.max_y, y);

    if (IsBorderTile(mask, x, y, size)) {
      region.border_tile_indices.push_back(index);
      ++region.border_tile_count;
    } else {
      ++region.inner_tile_count;
    }

    constexpr int kNeighborCount = 4;
    const int dx[kNeighborCount] = {1, -1, 0, 0};
    const int dy[kNeighborCount] = {0, 0, 1, -1};

    for (int neighbor = 0; neighbor < kNeighborCount; ++neighbor) {
      const int nx = x + dx[neighbor];
      const int ny = y + dy[neighbor];
      if (!IsMaskSet(mask, nx, ny, size)) {
        continue;
      }

      const int neighbor_index = ToIndex(nx, ny, size.width);
      const std::size_t visited_index =
          static_cast<std::size_t>(neighbor_index);
      if ((*visited)[visited_index] != 0) {
        continue;
      }

      (*visited)[visited_index] = 1;
      pending.push(neighbor_index);
    }
  }

  return region;
}

std::vector<MaskDescriptor> BuildMaskDescriptors(const SemanticMasks& masks) {
  return {
      {TerrainType::kOpenGround, &masks.open_ground},
      {TerrainType::kForest, &masks.forest},
      {TerrainType::kRoad, &masks.road},
      {TerrainType::kSwamp, &masks.swamp},
      {TerrainType::kWater, &masks.water},
      {TerrainType::kRuins, &masks.ruins},
      {TerrainType::kWall, &masks.wall},
      {TerrainType::kUnknown, &masks.unknown},
  };
}

}  // namespace

std::string TerrainRegion::Dump() const {
  return "TerrainRegion { id: " + std::to_string(id) +
         ", type: " + std::string(TerrainTypeToString(type)) +
         ", area: " + std::to_string(area) + ", bounds: [" +
         std::to_string(min_x) + "," + std::to_string(min_y) + ".." +
         std::to_string(max_x) + "," + std::to_string(max_y) + "]" +
         ", inner: " + std::to_string(inner_tile_count) +
         ", border: " + std::to_string(border_tile_count) + " }";
}

std::string TerrainRegionSummary::Dump() const {
  return "TerrainRegionSummary { total: " +
         std::to_string(total_regions) +
         ", open: " + std::to_string(open_ground_regions) +
         ", forest: " + std::to_string(forest_regions) +
         ", road: " + std::to_string(road_regions) +
         ", swamp: " + std::to_string(swamp_regions) +
         ", water: " + std::to_string(water_regions) +
         ", ruins: " + std::to_string(ruins_regions) +
         ", wall: " + std::to_string(wall_regions) +
         ", unknown: " + std::to_string(unknown_regions) +
         ", tiny: " + std::to_string(tiny_regions) +
         ", largest: " + std::to_string(largest_region_area) +
         ", largest_forest: " + std::to_string(largest_forest_area) +
         ", largest_open: " +
         std::to_string(largest_open_ground_area) + " }";
}

bool TerrainRegions::IsValid() const {
  if (size.width <= 0 || size.height <= 0) {
    return false;
  }

  return summary.total_regions == static_cast<int>(regions.size());
}

std::string TerrainRegions::Dump() const {
  return "TerrainRegions { size: " + std::to_string(size.width) + "x" +
         std::to_string(size.height) + ", " + summary.Dump() + " }";
}

TerrainRegions BuildTerrainRegions(const SemanticMasks& masks,
                                    std::string* error) {
  TerrainRegions terrain_regions;
  terrain_regions.size = masks.size;

  if (!masks.IsValid()) {
    if (error != nullptr) {
      *error = "semantic masks are invalid for terrain region building";
    }
    return terrain_regions;
  }

  const std::size_t expected_size =
      static_cast<std::size_t>(masks.size.width * masks.size.height);
  int next_region_id = 1;

  for (const MaskDescriptor& descriptor : BuildMaskDescriptors(masks)) {
    if (descriptor.mask == nullptr || descriptor.mask->size() != expected_size) {
      if (error != nullptr) {
        *error = "semantic mask size mismatch for terrain region building";
      }
      return terrain_regions;
    }

    std::vector<std::uint8_t> visited(expected_size, 0);
    for (std::size_t index = 0; index < expected_size; ++index) {
      if ((*descriptor.mask)[index] == 0 || visited[index] != 0) {
        continue;
      }

      TerrainRegion region = BuildRegionFromSeed(
          *descriptor.mask, descriptor.type, static_cast<int>(index),
          next_region_id, masks.size, &visited);
      ++next_region_id;
      UpdateSummaryForRegion(region, &terrain_regions.summary);
      terrain_regions.regions.push_back(std::move(region));
    }
  }

  return terrain_regions;
}

}  // namespace sar::visual_pipeline
