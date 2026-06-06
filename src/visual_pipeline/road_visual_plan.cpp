#include "visual_pipeline/road_visual_plan.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <string>
#include <string_view>
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

bool TextContains(std::string_view text, std::string_view needle) {
  return text.find(needle) != std::string_view::npos;
}

bool HasTag(const Route& route, std::string_view tag_name) {
  for (const std::string& tag : route.tags) {
    if (tag == tag_name) {
      return true;
    }
  }
  return false;
}

bool RouteIsMain(const Route& route) {
  return TextContains(route.type, "main") || HasTag(route, "primary") ||
         HasTag(route, "main_road") || HasTag(route, "critical");
}

bool RouteIsHidden(const Route& route) {
  if (TextContains(route.type, "hidden") || TextContains(route.type, "secret")) {
    return true;
  }
  for (const std::string& tag : route.tags) {
    if (tag == "hidden" || tag == "secret") {
      return true;
    }
  }
  return false;
}

std::uint8_t BandPriority(RoadVisualBand band) {
  switch (band) {
    case RoadVisualBand::kRuinApproach:
      return 6;
    case RoadVisualBand::kMudPatch:
      return 5;
    case RoadVisualBand::kRoadCore:
      return 4;
    case RoadVisualBand::kRoadSide:
      return 3;
    case RoadVisualBand::kTrampledGrass:
      return 2;
    case RoadVisualBand::kNone:
      return 0;
  }
  return 0;
}

void SetRoadBand(int x, int y, RoadVisualBand band, const LevelSize& size,
                 std::vector<std::uint8_t>* road_bands) {
  if (road_bands == nullptr || !IsInside(size, x, y)) {
    return;
  }
  const int index = ToIndex(size, x, y);
  const auto item = static_cast<std::size_t>(index);
  const RoadVisualBand current = static_cast<RoadVisualBand>((*road_bands)[item]);
  if (BandPriority(band) >= BandPriority(current)) {
    (*road_bands)[item] = static_cast<std::uint8_t>(band);
  }
}

void SetInfluence(int x, int y, std::uint8_t value, const LevelSize& size,
                  std::vector<std::uint8_t>* road_influence) {
  if (road_influence == nullptr || !IsInside(size, x, y)) {
    return;
  }
  const int index = ToIndex(size, x, y);
  auto& target = (*road_influence)[static_cast<std::size_t>(index)];
  target = std::max(target, value);
}

bool IsRoadDressingTerrain(TerrainType terrain) {
  return terrain == TerrainType::kOpenGround || terrain == TerrainType::kForest;
}

bool IsMudCandidate(int x, int y) {
  const std::size_t hash = (static_cast<std::size_t>(x) * 73856093U) ^
                           (static_cast<std::size_t>(y) * 19349663U) ^
                           0x9E3779B9U;
  return hash % 19U == 0U;
}

bool IsRoadCell(const LevelData& level, int x, int y) {
  if (!IsInside(level.size, x, y)) {
    return false;
  }
  const std::size_t item = static_cast<std::size_t>(ToIndex(level.size, x, y));
  return item < level.cells.size() &&
         level.cells[item].terrain == TerrainType::kRoad;
}

void PaintRoadTerrainCell(int x, int y, const LevelData& level,
                          RoadVisualPlan* plan) {
  if (plan == nullptr || !IsInside(level.size, x, y)) {
    return;
  }

  SetRoadBand(x, y, RoadVisualBand::kRoadCore, level.size, &plan->road_bands);
  SetInfluence(x, y, 3, level.size, &plan->route_influence);
  if (IsMudCandidate(x, y)) {
    SetRoadBand(x, y, RoadVisualBand::kMudPatch, level.size, &plan->road_bands);
  }

  for (int dy = -2; dy <= 2; ++dy) {
    for (int dx = -2; dx <= 2; ++dx) {
      if (dx == 0 && dy == 0) {
        continue;
      }
      const int nx = x + dx;
      const int ny = y + dy;
      if (!IsInside(level.size, nx, ny)) {
        continue;
      }
      const int distance_squared = dx * dx + dy * dy;
      if (distance_squared > 4) {
        continue;
      }
      const std::size_t item =
          static_cast<std::size_t>(ToIndex(level.size, nx, ny));
      if (item >= level.cells.size() ||
          !IsRoadDressingTerrain(level.cells[item].terrain)) {
        continue;
      }
      if (distance_squared <= 2) {
        SetRoadBand(nx, ny, RoadVisualBand::kRoadSide, level.size,
                    &plan->road_bands);
        SetInfluence(nx, ny, 2, level.size, &plan->route_influence);
      } else {
        SetRoadBand(nx, ny, RoadVisualBand::kTrampledGrass, level.size,
                    &plan->road_bands);
        SetInfluence(nx, ny, 1, level.size, &plan->route_influence);
      }
    }
  }
}

std::vector<std::uint8_t> BuildRuinInfluence(const SemanticMasks& masks) {
  std::vector<std::uint8_t> influence(
      static_cast<std::size_t>(CellCount(masks.size)), 0);
  for (int y = 0; y < masks.size.height; ++y) {
    for (int x = 0; x < masks.size.width; ++x) {
      const int index = ToIndex(masks.size, x, y);
      const auto item = static_cast<std::size_t>(index);
      if (masks.ruins[item] == 0 && masks.wall[item] == 0) {
        continue;
      }
      for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
          if (dx * dx + dy * dy > 4) {
            continue;
          }
          const int nx = x + dx;
          const int ny = y + dy;
          if (IsInside(masks.size, nx, ny)) {
            influence[static_cast<std::size_t>(ToIndex(masks.size, nx, ny))] = 1;
          }
        }
      }
    }
  }
  return influence;
}

bool HasAdjacentRoad(const LevelData& level, int x, int y) {
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      if (dx == 0 && dy == 0) {
        continue;
      }
      if (IsRoadCell(level, x + dx, y + dy)) {
        return true;
      }
    }
  }
  return false;
}

void ApplyRuinApproaches(const LevelData& level,
                         const std::vector<std::uint8_t>& ruin_influence,
                         RoadVisualPlan* plan) {
  if (plan == nullptr || ruin_influence.size() != plan->road_bands.size()) {
    return;
  }
  for (int y = 0; y < level.size.height; ++y) {
    for (int x = 0; x < level.size.width; ++x) {
      const std::size_t item =
          static_cast<std::size_t>(ToIndex(level.size, x, y));
      if (ruin_influence[item] == 0) {
        continue;
      }
      const RoadVisualBand current =
          static_cast<RoadVisualBand>(plan->road_bands[item]);
      if (current != RoadVisualBand::kRoadCore &&
          current != RoadVisualBand::kRoadSide &&
          current != RoadVisualBand::kMudPatch) {
        continue;
      }
      if (!IsRoadCell(level, x, y) && !HasAdjacentRoad(level, x, y)) {
        continue;
      }
      plan->road_bands[item] = static_cast<std::uint8_t>(
          RoadVisualBand::kRuinApproach);
    }
  }
}

void CountRoadBand(std::uint8_t value, RoadVisualSummary* summary) {
  if (summary == nullptr) {
    return;
  }
  switch (static_cast<RoadVisualBand>(value)) {
    case RoadVisualBand::kRoadCore:
      ++summary->road_core_tiles;
      break;
    case RoadVisualBand::kRoadSide:
      ++summary->road_side_tiles;
      break;
    case RoadVisualBand::kTrampledGrass:
      ++summary->trampled_grass_tiles;
      break;
    case RoadVisualBand::kMudPatch:
      ++summary->mud_patch_tiles;
      break;
    case RoadVisualBand::kRuinApproach:
      ++summary->ruin_approach_tiles;
      break;
    case RoadVisualBand::kNone:
      break;
  }
}

void CountRoutes(const LevelData& level, RoadVisualSummary* summary) {
  if (summary == nullptr) {
    return;
  }
  summary->route_count = static_cast<int>(level.routes.size());
  for (const Route& route : level.routes) {
    if (RouteIsMain(route)) {
      ++summary->main_route_count;
    } else if (RouteIsHidden(route)) {
      ++summary->hidden_route_count;
    } else {
      ++summary->side_route_count;
    }
  }
}

void RecountSummary(const LevelData& level, RoadVisualPlan* plan) {
  if (plan == nullptr) {
    return;
  }
  const int route_count = plan->summary.route_count;
  const int main_route_count = plan->summary.main_route_count;
  const int side_route_count = plan->summary.side_route_count;
  const int hidden_route_count = plan->summary.hidden_route_count;
  plan->summary = {};
  plan->summary.route_count = route_count;
  plan->summary.main_route_count = main_route_count;
  plan->summary.side_route_count = side_route_count;
  plan->summary.hidden_route_count = hidden_route_count;
  plan->summary.routes_used_for_visual_roads = false;
  for (std::size_t i = 0; i < plan->road_bands.size(); ++i) {
    CountRoadBand(plan->road_bands[i], &plan->summary);
    if (plan->route_influence[i] != 0) {
      ++plan->summary.route_influenced_tiles;
    }
  }
  for (const RuntimeCell& cell : level.cells) {
    if (cell.terrain == TerrainType::kRoad) {
      ++plan->summary.terrain_road_tiles;
    }
  }
  plan->summary.road_dressing_tiles =
      plan->summary.road_side_tiles + plan->summary.trampled_grass_tiles +
      plan->summary.mud_patch_tiles + plan->summary.ruin_approach_tiles;
}

}  // namespace

const char* RoadVisualBandName(RoadVisualBand band) {
  switch (band) {
    case RoadVisualBand::kNone:
      return "none";
    case RoadVisualBand::kTrampledGrass:
      return "trampled_grass";
    case RoadVisualBand::kRoadSide:
      return "road_side";
    case RoadVisualBand::kRoadCore:
      return "road_core";
    case RoadVisualBand::kMudPatch:
      return "mud_patch";
    case RoadVisualBand::kRuinApproach:
      return "ruin_approach";
  }
  return "none";
}

std::string RoadVisualSummary::Dump() const {
  return "RoadVisualSummary { routes=" + std::to_string(route_count) +
         ", routes_used_for_visual_roads=" +
         std::string(routes_used_for_visual_roads ? "true" : "false") +
         ", terrain_road=" + std::to_string(terrain_road_tiles) +
         ", dressing=" + std::to_string(road_dressing_tiles) +
         ", main=" + std::to_string(main_route_count) +
         ", side=" + std::to_string(side_route_count) +
         ", hidden=" + std::to_string(hidden_route_count) +
         ", core=" + std::to_string(road_core_tiles) +
         ", side_band=" + std::to_string(road_side_tiles) +
         ", trampled=" + std::to_string(trampled_grass_tiles) +
         ", mud=" + std::to_string(mud_patch_tiles) +
         ", ruin_approach=" + std::to_string(ruin_approach_tiles) +
         ", influenced=" + std::to_string(route_influenced_tiles) + " }";
}

bool RoadVisualPlan::IsValid() const {
  const int expected_size = CellCount(size);
  if (expected_size <= 0) {
    return false;
  }
  const std::size_t expected = static_cast<std::size_t>(expected_size);
  return road_bands.size() == expected && route_influence.size() == expected;
}

std::string RoadVisualPlan::Dump() const {
  return "RoadVisualPlan { size=" + std::to_string(size.width) + "x" +
         std::to_string(size.height) + ", " + summary.Dump() + " }";
}

RoadVisualPlan BuildRoadVisualPlan(const LevelData& level,
                                   const SemanticMasks& masks,
                                   std::string* error) {
  RoadVisualPlan plan;
  plan.size = level.size;
  const int cell_count = CellCount(level.size);
  if (cell_count <= 0) {
    if (error != nullptr) {
      *error = "level size is invalid for road visual planning";
    }
    return plan;
  }
  if (!masks.IsValid()) {
    if (error != nullptr) {
      *error = "semantic masks are invalid for road visual planning";
    }
    return plan;
  }

  const std::size_t expected_size = static_cast<std::size_t>(cell_count);
  plan.road_bands.assign(expected_size, 0);
  plan.route_influence.assign(expected_size, 0);
  CountRoutes(level, &plan.summary);

  for (int y = 0; y < level.size.height; ++y) {
    for (int x = 0; x < level.size.width; ++x) {
      if (IsRoadCell(level, x, y)) {
        PaintRoadTerrainCell(x, y, level, &plan);
      }
    }
  }

  ApplyRuinApproaches(level, BuildRuinInfluence(masks), &plan);
  RecountSummary(level, &plan);
  return plan;
}

}  // namespace sar::visual_pipeline
