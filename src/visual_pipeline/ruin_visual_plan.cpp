#include "visual_pipeline/ruin_visual_plan.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <queue>
#include <string>
#include <utility>
#include <vector>

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

bool IsWall(const SemanticMasks& masks, int x, int y) {
  if (!IsInside(masks.size, x, y)) {
    return false;
  }
  return masks.wall[ToItem(masks.size, x, y)] != 0;
}

bool IsRuinFloor(const SemanticMasks& masks, int x, int y) {
  if (!IsInside(masks.size, x, y)) {
    return false;
  }
  return masks.ruins[ToItem(masks.size, x, y)] != 0;
}

bool IsRuinSeed(const SemanticMasks& masks, int x, int y) {
  return IsWall(masks, x, y) || IsRuinFloor(masks, x, y);
}

bool IsOpenForRuinDressing(const LevelData& level, int x, int y) {
  if (!IsInside(level.size, x, y)) {
    return false;
  }
  const RuntimeCell& cell = level.cells[ToItem(level.size, x, y)];
  return cell.terrain == TerrainType::kOpenGround ||
         cell.terrain == TerrainType::kRoad ||
         cell.terrain == TerrainType::kRuins;
}

bool IsRoad(const LevelData& level, int x, int y) {
  if (!IsInside(level.size, x, y)) {
    return false;
  }
  return level.cells[ToItem(level.size, x, y)].terrain == TerrainType::kRoad;
}

std::uint32_t TileHash(int x, int y, std::uint32_t salt) {
  return (static_cast<std::uint32_t>(x) * 1103515245U) ^
         (static_cast<std::uint32_t>(y) * 2654435761U) ^ salt;
}

bool LooksBroken(int x, int y) {
  return TileHash(x, y, 0x7F4A7C15U) % 9U == 0U;
}

bool LooksOvergrown(int x, int y) {
  return TileHash(x, y, 0x9E3779B9U) % 6U == 0U;
}

bool LooksRubble(int x, int y) {
  return TileHash(x, y, 0xA24BAED5U) % 4U == 0U;
}

void SetTile(const LevelSize& size, int x, int y, RuinVisualTile tile,
             std::vector<std::uint8_t>* tiles) {
  if (tiles == nullptr || !IsInside(size, x, y)) {
    return;
  }
  const std::size_t item = ToItem(size, x, y);
  const auto current = static_cast<RuinVisualTile>((*tiles)[item]);
  if (static_cast<std::uint8_t>(tile) >= static_cast<std::uint8_t>(current)) {
    (*tiles)[item] = static_cast<std::uint8_t>(tile);
  }
}

RuinVisualTile ClassifyWallTile(const SemanticMasks& masks, int x, int y) {
  const bool north = IsWall(masks, x, y - 1);
  const bool south = IsWall(masks, x, y + 1);
  const bool west = IsWall(masks, x - 1, y);
  const bool east = IsWall(masks, x + 1, y);
  const int neighbor_count = static_cast<int>(north) + static_cast<int>(south) +
                             static_cast<int>(west) + static_cast<int>(east);

  if (neighbor_count <= 1) {
    return RuinVisualTile::kWallEndcap;
  }
  const bool vertical = north && south;
  const bool horizontal = west && east;
  if (neighbor_count == 2 && !vertical && !horizontal) {
    return RuinVisualTile::kWallCorner;
  }
  if (LooksBroken(x, y)) {
    return RuinVisualTile::kWallBroken;
  }
  return RuinVisualTile::kWallIntact;
}

void BuildSiteIds(const SemanticMasks& masks, RuinVisualPlan* plan) {
  if (plan == nullptr) {
    return;
  }

  std::uint16_t next_site_id = 1;
  std::queue<std::pair<int, int>> queue;
  for (int y = 0; y < masks.size.height; ++y) {
    for (int x = 0; x < masks.size.width; ++x) {
      const std::size_t item = ToItem(masks.size, x, y);
      if (!IsRuinSeed(masks, x, y) || plan->site_ids[item] != 0) {
        continue;
      }

      plan->site_ids[item] = next_site_id;
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
            if (!IsInside(masks.size, nx, ny) || !IsRuinSeed(masks, nx, ny)) {
              continue;
            }
            const std::size_t neighbor_item = ToItem(masks.size, nx, ny);
            if (plan->site_ids[neighbor_item] != 0) {
              continue;
            }
            plan->site_ids[neighbor_item] = next_site_id;
            queue.push({nx, ny});
          }
        }
      }

      if (next_site_id < UINT16_MAX) {
        ++next_site_id;
      }
    }
  }

  plan->summary.site_count = static_cast<int>(next_site_id) - 1;
}

void PaintSourceRuinTiles(const LevelData& level, const SemanticMasks& masks,
                          RuinVisualPlan* plan) {
  if (plan == nullptr) {
    return;
  }

  for (int y = 0; y < level.size.height; ++y) {
    for (int x = 0; x < level.size.width; ++x) {
      const std::size_t item = ToItem(level.size, x, y);
      if (masks.wall[item] != 0) {
        ++plan->summary.source_wall_tiles;
        SetTile(level.size, x, y, ClassifyWallTile(masks, x, y),
                &plan->tiles);
        continue;
      }
      if (masks.ruins[item] != 0) {
        ++plan->summary.source_ruin_tiles;
        SetTile(level.size, x, y,
                LooksOvergrown(x, y) ? RuinVisualTile::kOvergrownFloor
                                     : RuinVisualTile::kCrackedFloor,
                &plan->tiles);
      }
    }
  }
}

void PaintRuinDressing(const LevelData& level, const SemanticMasks& masks,
                       RuinVisualPlan* plan) {
  if (plan == nullptr) {
    return;
  }

  for (int y = 0; y < level.size.height; ++y) {
    for (int x = 0; x < level.size.width; ++x) {
      if (!IsRuinSeed(masks, x, y)) {
        continue;
      }

      const std::uint16_t site_id = plan->site_ids[ToItem(level.size, x, y)];
      const auto source_wall_role =
          static_cast<RuinVisualTile>(plan->tiles[ToItem(level.size, x, y)]);
      for (int direction = 0; direction < 4; ++direction) {
        const int dx = direction == 0 ? -1 : (direction == 1 ? 1 : 0);
        const int dy = direction == 2 ? -1 : (direction == 3 ? 1 : 0);
        const int nx = x + dx;
        const int ny = y + dy;
        if (!IsOpenForRuinDressing(level, nx, ny)) {
          continue;
        }

        const std::size_t neighbor_item = ToItem(level.size, nx, ny);
        if (plan->tiles[neighbor_item] != 0) {
          continue;
        }
        if (site_id != 0 && plan->site_ids[neighbor_item] == 0) {
          plan->site_ids[neighbor_item] = site_id;
        }

        if (IsRoad(level, nx, ny)) {
          SetTile(level.size, nx, ny, RuinVisualTile::kEntrance,
                  &plan->tiles);
          continue;
        }

        if (IsWall(masks, x, y) && LooksRubble(nx, ny) &&
            (source_wall_role == RuinVisualTile::kWallBroken ||
             source_wall_role == RuinVisualTile::kWallCorner ||
             source_wall_role == RuinVisualTile::kWallEndcap)) {
          SetTile(level.size, nx, ny, RuinVisualTile::kRubble, &plan->tiles);
          continue;
        }

        if (IsRuinFloor(masks, x, y) && LooksOvergrown(nx, ny)) {
          SetTile(level.size, nx, ny, RuinVisualTile::kOvergrownFloor,
                  &plan->tiles);
        }
      }
    }
  }
}

void CountTile(std::uint8_t value, RuinVisualSummary* summary) {
  if (summary == nullptr || value == 0) {
    return;
  }
  ++summary->visual_tiles;
  switch (static_cast<RuinVisualTile>(value)) {
    case RuinVisualTile::kCrackedFloor:
      ++summary->cracked_floor_tiles;
      break;
    case RuinVisualTile::kOvergrownFloor:
      ++summary->overgrown_floor_tiles;
      break;
    case RuinVisualTile::kWallIntact:
      ++summary->wall_intact_tiles;
      break;
    case RuinVisualTile::kWallBroken:
      ++summary->wall_broken_tiles;
      break;
    case RuinVisualTile::kWallCorner:
      ++summary->wall_corner_tiles;
      break;
    case RuinVisualTile::kWallEndcap:
      ++summary->wall_endcap_tiles;
      break;
    case RuinVisualTile::kRubble:
      ++summary->rubble_tiles;
      break;
    case RuinVisualTile::kEntrance:
      ++summary->entrance_tiles;
      break;
    case RuinVisualTile::kNone:
      break;
  }
}

void RecountSummary(RuinVisualPlan* plan) {
  if (plan == nullptr) {
    return;
  }
  const int site_count = plan->summary.site_count;
  const int source_ruin_tiles = plan->summary.source_ruin_tiles;
  const int source_wall_tiles = plan->summary.source_wall_tiles;
  plan->summary = {};
  plan->summary.site_count = site_count;
  plan->summary.source_ruin_tiles = source_ruin_tiles;
  plan->summary.source_wall_tiles = source_wall_tiles;
  for (const std::uint8_t value : plan->tiles) {
    CountTile(value, &plan->summary);
  }
}

}  // namespace

const char* RuinVisualTileName(RuinVisualTile tile) {
  switch (tile) {
    case RuinVisualTile::kNone:
      return "none";
    case RuinVisualTile::kCrackedFloor:
      return "cracked_floor";
    case RuinVisualTile::kOvergrownFloor:
      return "overgrown_floor";
    case RuinVisualTile::kWallIntact:
      return "wall_intact";
    case RuinVisualTile::kWallBroken:
      return "wall_broken";
    case RuinVisualTile::kWallCorner:
      return "wall_corner";
    case RuinVisualTile::kWallEndcap:
      return "wall_endcap";
    case RuinVisualTile::kRubble:
      return "rubble";
    case RuinVisualTile::kEntrance:
      return "entrance";
  }
  return "none";
}

std::string RuinVisualSummary::Dump() const {
  return "RuinVisualSummary { sites=" + std::to_string(site_count) +
         ", source_ruins=" + std::to_string(source_ruin_tiles) +
         ", source_walls=" + std::to_string(source_wall_tiles) +
         ", cracked=" + std::to_string(cracked_floor_tiles) +
         ", overgrown=" + std::to_string(overgrown_floor_tiles) +
         ", intact=" + std::to_string(wall_intact_tiles) +
         ", broken=" + std::to_string(wall_broken_tiles) +
         ", corners=" + std::to_string(wall_corner_tiles) +
         ", endcaps=" + std::to_string(wall_endcap_tiles) +
         ", rubble=" + std::to_string(rubble_tiles) +
         ", entrances=" + std::to_string(entrance_tiles) +
         ", visual=" + std::to_string(visual_tiles) + " }";
}

bool RuinVisualPlan::IsValid() const {
  const int expected_size = CellCount(size);
  if (expected_size <= 0) {
    return false;
  }
  const std::size_t expected = static_cast<std::size_t>(expected_size);
  return tiles.size() == expected && site_ids.size() == expected;
}

std::string RuinVisualPlan::Dump() const {
  return "RuinVisualPlan { size=" + std::to_string(size.width) + "x" +
         std::to_string(size.height) + ", " + summary.Dump() + " }";
}

RuinVisualPlan BuildRuinVisualPlan(const LevelData& level,
                                   const SemanticMasks& masks,
                                   std::string* error) {
  RuinVisualPlan plan;
  plan.size = level.size;
  const int cell_count = CellCount(level.size);
  if (cell_count <= 0) {
    if (error != nullptr) {
      *error = "level size is invalid for ruin visual planning";
    }
    return plan;
  }
  if (!masks.IsValid()) {
    if (error != nullptr) {
      *error = "semantic masks are invalid for ruin visual planning";
    }
    return plan;
  }

  const std::size_t expected_size = static_cast<std::size_t>(cell_count);
  plan.tiles.assign(expected_size, 0);
  plan.site_ids.assign(expected_size, 0);

  BuildSiteIds(masks, &plan);
  PaintSourceRuinTiles(level, masks, &plan);
  PaintRuinDressing(level, masks, &plan);
  RecountSummary(&plan);
  return plan;
}

}  // namespace sar::visual_pipeline
