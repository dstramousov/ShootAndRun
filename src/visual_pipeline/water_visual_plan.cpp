#include "visual_pipeline/water_visual_plan.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <queue>
#include <string>
#include <utility>

namespace sar::visual_pipeline {
namespace {

int CellCount(const LevelSize& size) {
  if (size.width <= 0 || size.height <= 0) {
    return 0;
  }
  return size.width * size.height;
}

bool IsInside(const LevelSize& size, int x, int y) {
  return x >= 0 && y >= 0 && x < size.width && y < size.height;
}

int ToIndex(const LevelSize& size, int x, int y) {
  return y * size.width + x;
}

std::size_t ToItem(const LevelSize& size, int x, int y) {
  return static_cast<std::size_t>(ToIndex(size, x, y));
}

bool IsWaterLike(const SemanticMasks& masks, int x, int y) {
  if (!IsInside(masks.size, x, y)) {
    return false;
  }
  const std::size_t item = ToItem(masks.size, x, y);
  return masks.water[item] != 0 || masks.swamp[item] != 0;
}

bool IsOpenDressingTerrain(TerrainType terrain) {
  return terrain == TerrainType::kOpenGround || terrain == TerrainType::kForest ||
         terrain == TerrainType::kRoad || terrain == TerrainType::kRuins;
}

bool IsRoadTerrain(const LevelData& level, int x, int y) {
  if (!IsInside(level.size, x, y)) {
    return false;
  }
  return level.cells[ToItem(level.size, x, y)].terrain == TerrainType::kRoad;
}

bool HasAdjacentWaterLike(const SemanticMasks& masks, int x, int y) {
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      if (dx == 0 && dy == 0) {
        continue;
      }
      if (IsWaterLike(masks, x + dx, y + dy)) {
        return true;
      }
    }
  }
  return false;
}

bool IsWaterInterior(const SemanticMasks& masks, int x, int y) {
  if (!IsWaterLike(masks, x, y)) {
    return false;
  }
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      if (dx == 0 && dy == 0) {
        continue;
      }
      if (!IsWaterLike(masks, x + dx, y + dy)) {
        return false;
      }
    }
  }
  return true;
}

std::uint32_t TileHash(int x, int y, std::uint32_t salt) {
  return (static_cast<std::uint32_t>(x) * 1103515245U) ^
         (static_cast<std::uint32_t>(y) * 2654435761U) ^ salt;
}

bool WantsReed(int x, int y) {
  return TileHash(x, y, 0xA24BAED5U) % 4U == 0U;
}

bool WantsWetGrass(int x, int y) {
  return TileHash(x, y, 0x7F4A7C15U) % 3U != 0U;
}

std::uint8_t TilePriority(WaterVisualTile tile) {
  switch (tile) {
    case WaterVisualTile::kWaterCore:
      return 8;
    case WaterVisualTile::kWaterEdge:
      return 7;
    case WaterVisualTile::kCrossing:
      return 6;
    case WaterVisualTile::kReedZone:
      return 5;
    case WaterVisualTile::kMudRing:
      return 4;
    case WaterVisualTile::kWetGrass:
      return 3;
    case WaterVisualTile::kNone:
      return 0;
  }
  return 0;
}

void SetTile(const LevelSize& size, int x, int y, WaterVisualTile tile,
             std::vector<std::uint8_t>* tiles) {
  if (tiles == nullptr || !IsInside(size, x, y)) {
    return;
  }
  const std::size_t item = ToItem(size, x, y);
  const auto current = static_cast<WaterVisualTile>((*tiles)[item]);
  if (TilePriority(tile) >= TilePriority(current)) {
    (*tiles)[item] = static_cast<std::uint8_t>(tile);
  }
}

void BuildRegionIds(const SemanticMasks& masks, WaterVisualPlan* plan) {
  if (plan == nullptr) {
    return;
  }

  std::uint16_t next_region_id = 1;
  std::queue<std::pair<int, int>> queue;
  for (int y = 0; y < masks.size.height; ++y) {
    for (int x = 0; x < masks.size.width; ++x) {
      if (!IsWaterLike(masks, x, y)) {
        continue;
      }
      const std::size_t item = ToItem(masks.size, x, y);
      if (plan->region_ids[item] != 0) {
        continue;
      }

      plan->region_ids[item] = next_region_id;
      queue.push({x, y});
      while (!queue.empty()) {
        const auto [cx, cy] = queue.front();
        queue.pop();
        for (int dy = -1; dy <= 1; ++dy) {
          for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) {
              continue;
            }
            const int nx = cx + dx;
            const int ny = cy + dy;
            if (!IsWaterLike(masks, nx, ny)) {
              continue;
            }
            const std::size_t neighbor_item = ToItem(masks.size, nx, ny);
            if (plan->region_ids[neighbor_item] != 0) {
              continue;
            }
            plan->region_ids[neighbor_item] = next_region_id;
            queue.push({nx, ny});
          }
        }
      }

      if (next_region_id < UINT16_MAX) {
        ++next_region_id;
      }
    }
  }
  plan->summary.water_region_count = static_cast<int>(next_region_id) - 1;
}

void PaintWaterSources(const SemanticMasks& masks, WaterVisualPlan* plan) {
  if (plan == nullptr) {
    return;
  }

  for (int y = 0; y < masks.size.height; ++y) {
    for (int x = 0; x < masks.size.width; ++x) {
      const std::size_t item = ToItem(masks.size, x, y);
      if (masks.water[item] != 0) {
        ++plan->summary.source_water_tiles;
      }
      if (masks.swamp[item] != 0) {
        ++plan->summary.source_swamp_tiles;
      }
      if (!IsWaterLike(masks, x, y)) {
        continue;
      }
      ++plan->summary.water_like_tiles;
      SetTile(masks.size, x, y,
              IsWaterInterior(masks, x, y) ? WaterVisualTile::kWaterCore
                                           : WaterVisualTile::kWaterEdge,
              &plan->tiles);
    }
  }
}

void PaintWaterDressing(const LevelData& level, const SemanticMasks& masks,
                        WaterVisualPlan* plan) {
  if (plan == nullptr) {
    return;
  }

  for (int y = 0; y < level.size.height; ++y) {
    for (int x = 0; x < level.size.width; ++x) {
      if (!IsWaterLike(masks, x, y)) {
        continue;
      }

      for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
          if (dx == 0 && dy == 0) {
            continue;
          }
          const int distance_squared = dx * dx + dy * dy;
          if (distance_squared > 5) {
            continue;
          }

          const int nx = x + dx;
          const int ny = y + dy;
          if (!IsInside(level.size, nx, ny) || IsWaterLike(masks, nx, ny)) {
            continue;
          }

          const std::size_t item = ToItem(level.size, nx, ny);
          if (item >= level.cells.size() ||
              !IsOpenDressingTerrain(level.cells[item].terrain)) {
            continue;
          }

          if (IsRoadTerrain(level, nx, ny) && distance_squared <= 2) {
            SetTile(level.size, nx, ny, WaterVisualTile::kCrossing,
                    &plan->tiles);
            continue;
          }

          if (distance_squared <= 2) {
            if (WantsReed(nx, ny) && HasAdjacentWaterLike(masks, nx, ny)) {
              SetTile(level.size, nx, ny, WaterVisualTile::kReedZone,
                      &plan->tiles);
            } else {
              SetTile(level.size, nx, ny, WaterVisualTile::kMudRing,
                      &plan->tiles);
            }
            continue;
          }

          if (WantsWetGrass(nx, ny)) {
            SetTile(level.size, nx, ny, WaterVisualTile::kWetGrass,
                    &plan->tiles);
          }
        }
      }
    }
  }
}

void CountTile(std::uint8_t value, WaterVisualSummary* summary) {
  if (summary == nullptr || value == 0) {
    return;
  }
  ++summary->visual_tiles;
  switch (static_cast<WaterVisualTile>(value)) {
    case WaterVisualTile::kWaterCore:
      ++summary->water_core_tiles;
      break;
    case WaterVisualTile::kWaterEdge:
      ++summary->water_edge_tiles;
      break;
    case WaterVisualTile::kMudRing:
      ++summary->mud_ring_tiles;
      break;
    case WaterVisualTile::kWetGrass:
      ++summary->wet_grass_tiles;
      break;
    case WaterVisualTile::kReedZone:
      ++summary->reed_zone_tiles;
      break;
    case WaterVisualTile::kCrossing:
      ++summary->crossing_tiles;
      break;
    case WaterVisualTile::kNone:
      break;
  }
}

void RecountSummary(WaterVisualPlan* plan) {
  if (plan == nullptr) {
    return;
  }
  const int source_water_tiles = plan->summary.source_water_tiles;
  const int source_swamp_tiles = plan->summary.source_swamp_tiles;
  const int water_like_tiles = plan->summary.water_like_tiles;
  const int water_region_count = plan->summary.water_region_count;
  plan->summary = {};
  plan->summary.source_water_tiles = source_water_tiles;
  plan->summary.source_swamp_tiles = source_swamp_tiles;
  plan->summary.water_like_tiles = water_like_tiles;
  plan->summary.water_region_count = water_region_count;
  for (const std::uint8_t value : plan->tiles) {
    CountTile(value, &plan->summary);
  }
}

}  // namespace

const char* WaterVisualTileName(WaterVisualTile tile) {
  switch (tile) {
    case WaterVisualTile::kNone:
      return "none";
    case WaterVisualTile::kWetGrass:
      return "wet_grass";
    case WaterVisualTile::kMudRing:
      return "mud_ring";
    case WaterVisualTile::kReedZone:
      return "reed_zone";
    case WaterVisualTile::kCrossing:
      return "crossing";
    case WaterVisualTile::kWaterEdge:
      return "water_edge";
    case WaterVisualTile::kWaterCore:
      return "water_core";
  }
  return "none";
}

std::string WaterVisualSummary::Dump() const {
  return "WaterVisualSummary { source_water=" +
         std::to_string(source_water_tiles) +
         ", source_swamp=" + std::to_string(source_swamp_tiles) +
         ", water_like=" + std::to_string(water_like_tiles) +
         ", regions=" + std::to_string(water_region_count) +
         ", core=" + std::to_string(water_core_tiles) +
         ", edge=" + std::to_string(water_edge_tiles) +
         ", mud=" + std::to_string(mud_ring_tiles) +
         ", wet_grass=" + std::to_string(wet_grass_tiles) +
         ", reeds=" + std::to_string(reed_zone_tiles) +
         ", crossings=" + std::to_string(crossing_tiles) +
         ", visual=" + std::to_string(visual_tiles) + " }";
}

bool WaterVisualPlan::IsValid() const {
  const int expected_size = CellCount(size);
  if (expected_size <= 0) {
    return false;
  }
  const std::size_t expected = static_cast<std::size_t>(expected_size);
  return tiles.size() == expected && region_ids.size() == expected;
}

std::string WaterVisualPlan::Dump() const {
  return "WaterVisualPlan { size=" + std::to_string(size.width) + "x" +
         std::to_string(size.height) + ", " + summary.Dump() + " }";
}

WaterVisualPlan BuildWaterVisualPlan(const LevelData& level,
                                     const SemanticMasks& masks,
                                     std::string* error) {
  WaterVisualPlan plan;
  plan.size = level.size;
  const int cell_count = CellCount(level.size);
  if (cell_count <= 0) {
    if (error != nullptr) {
      *error = "level size is invalid for water visual planning";
    }
    return plan;
  }
  if (!masks.IsValid()) {
    if (error != nullptr) {
      *error = "semantic masks are invalid for water visual planning";
    }
    return plan;
  }

  const std::size_t expected_size = static_cast<std::size_t>(cell_count);
  plan.tiles.assign(expected_size, 0);
  plan.region_ids.assign(expected_size, 0);

  BuildRegionIds(masks, &plan);
  PaintWaterSources(masks, &plan);
  PaintWaterDressing(level, masks, &plan);
  RecountSummary(&plan);
  return plan;
}

}  // namespace sar::visual_pipeline
