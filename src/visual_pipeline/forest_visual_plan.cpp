#include "visual_pipeline/forest_visual_plan.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <queue>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace sar::visual_pipeline {
namespace {

constexpr int kNoRegionArea = 0;
constexpr int kTinyOpenRegionArea = 32;
constexpr int kTinyForestRegionArea = 8;
constexpr int kForestMassMergeGapTiles = 3;
constexpr int kForestEdgeMaxDistance = 2;
constexpr int kForestMidMaxDistance = 5;

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

struct DisjointSet {
  std::vector<int> parent;

  explicit DisjointSet(int size) : parent(static_cast<std::size_t>(size), 0) {
    for (int i = 0; i < size; ++i) {
      parent[static_cast<std::size_t>(i)] = i;
    }
  }

  int Find(int item) {
    int& parent_item = parent[static_cast<std::size_t>(item)];
    if (parent_item == item) {
      return item;
    }
    parent_item = Find(parent_item);
    return parent_item;
  }

  void Unite(int a, int b) {
    const int root_a = Find(a);
    const int root_b = Find(b);
    if (root_a != root_b) {
      parent[static_cast<std::size_t>(root_b)] = root_a;
    }
  }
};

int RegionGap(const TerrainRegion& a, const TerrainRegion& b) {
  const int gap_x = std::max({0, a.min_x - b.max_x - 1,
                              b.min_x - a.max_x - 1});
  const int gap_y = std::max({0, a.min_y - b.max_y - 1,
                              b.min_y - a.max_y - 1});
  return std::max(gap_x, gap_y);
}

bool RouteIsMain(const Route& route) {
  if (TextContains(route.type, "main")) {
    return true;
  }
  for (const std::string& tag : route.tags) {
    if (tag == "primary" || tag == "main_road" || tag == "critical") {
      return true;
    }
  }
  return false;
}

void SetInfluencePoint(int x, int y, int radius, std::uint8_t value,
                       const LevelSize& size,
                       std::vector<std::uint8_t>* influence) {
  if (influence == nullptr) {
    return;
  }
  for (int dy = -radius; dy <= radius; ++dy) {
    for (int dx = -radius; dx <= radius; ++dx) {
      if (dx * dx + dy * dy > radius * radius) {
        continue;
      }
      const int px = x + dx;
      const int py = y + dy;
      if (!IsInside(size, px, py)) {
        continue;
      }
      const int index = ToIndex(size, px, py);
      (*influence)[static_cast<std::size_t>(index)] = std::max(
          (*influence)[static_cast<std::size_t>(index)], value);
    }
  }
}

void DrawInfluenceLine(int x0, int y0, int x1, int y1, int radius,
                       std::uint8_t value, const LevelSize& size,
                       std::vector<std::uint8_t>* influence) {
  const int dx = std::abs(x1 - x0);
  const int sx = x0 < x1 ? 1 : -1;
  const int dy = -std::abs(y1 - y0);
  const int sy = y0 < y1 ? 1 : -1;
  int error_value = dx + dy;

  int x = x0;
  int y = y0;
  while (true) {
    SetInfluencePoint(x, y, radius, value, size, influence);
    if (x == x1 && y == y1) {
      break;
    }
    const int doubled_error = 2 * error_value;
    if (doubled_error >= dy) {
      error_value += dy;
      x += sx;
    }
    if (doubled_error <= dx) {
      error_value += dx;
      y += sy;
    }
  }
}

std::vector<std::uint8_t> BuildRouteInfluence(const LevelData& level) {
  std::vector<std::uint8_t> influence(
      static_cast<std::size_t>(CellCount(level.size)), 0);
  for (const Route& route : level.routes) {
    if (route.waypoints.empty()) {
      continue;
    }
    const bool main_route = RouteIsMain(route);
    const int radius = main_route ? 4 : 2;
    const std::uint8_t value = main_route ? 3 : 2;
    for (std::size_t i = 1; i < route.waypoints.size(); ++i) {
      const RoutePoint& previous = route.waypoints[i - 1];
      const RoutePoint& current = route.waypoints[i];
      DrawInfluenceLine(previous.x, previous.y, current.x, current.y, radius,
                        value, level.size, &influence);
    }
    for (const RoutePoint& point : route.waypoints) {
      SetInfluencePoint(point.x, point.y, main_route ? 5 : 3, value,
                        level.size, &influence);
    }
  }
  return influence;
}

void BuildForestRegionAreaLookup(const TerrainRegions& regions,
                                 std::vector<int>* forest_region_area) {
  if (forest_region_area == nullptr) {
    return;
  }
  for (const TerrainRegion& region : regions.regions) {
    if (region.type != TerrainType::kForest) {
      continue;
    }
    for (const int index : region.tile_indices) {
      if (index < 0 || index >= static_cast<int>(forest_region_area->size())) {
        continue;
      }
      (*forest_region_area)[static_cast<std::size_t>(index)] = region.area;
    }
  }
}

int BuildForestMassGroups(const TerrainRegions& regions, const LevelSize& size,
                          std::vector<std::uint16_t>* forest_mass_groups) {
  if (forest_mass_groups == nullptr) {
    return 0;
  }

  std::vector<const TerrainRegion*> forest_regions;
  for (const TerrainRegion& region : regions.regions) {
    if (region.type == TerrainType::kForest &&
        region.area > kTinyForestRegionArea) {
      forest_regions.push_back(&region);
    }
  }

  if (forest_regions.empty()) {
    return 0;
  }

  DisjointSet groups(static_cast<int>(forest_regions.size()));
  for (std::size_t i = 0; i < forest_regions.size(); ++i) {
    for (std::size_t j = i + 1; j < forest_regions.size(); ++j) {
      if (RegionGap(*forest_regions[i], *forest_regions[j]) <=
          kForestMassMergeGapTiles) {
        groups.Unite(static_cast<int>(i), static_cast<int>(j));
      }
    }
  }

  std::vector<int> root_to_group(forest_regions.size(), 0);
  int next_group = 1;
  for (std::size_t i = 0; i < forest_regions.size(); ++i) {
    const int root = groups.Find(static_cast<int>(i));
    int& group_id = root_to_group[static_cast<std::size_t>(root)];
    if (group_id == 0) {
      group_id = next_group;
      ++next_group;
    }

    for (const int index : forest_regions[i]->tile_indices) {
      if (index < 0 || index >= CellCount(size)) {
        continue;
      }
      (*forest_mass_groups)[static_cast<std::size_t>(index)] =
          static_cast<std::uint16_t>(group_id);
    }
  }

  return next_group - 1;
}

void BuildOpenRegionAreaLookup(const TerrainRegions& regions,
                               std::vector<int>* open_region_area) {
  if (open_region_area == nullptr) {
    return;
  }
  for (const TerrainRegion& region : regions.regions) {
    if (region.type != TerrainType::kOpenGround) {
      continue;
    }
    for (const int index : region.tile_indices) {
      if (index < 0 || index >= static_cast<int>(open_region_area->size())) {
        continue;
      }
      (*open_region_area)[static_cast<std::size_t>(index)] = region.area;
    }
  }
}

void MarkRadius(int center_x, int center_y, int radius, const LevelSize& size,
                std::vector<std::uint8_t>* mask) {
  if (mask == nullptr) {
    return;
  }
  for (int dy = -radius; dy <= radius; ++dy) {
    for (int dx = -radius; dx <= radius; ++dx) {
      if (dx * dx + dy * dy > radius * radius) {
        continue;
      }
      const int x = center_x + dx;
      const int y = center_y + dy;
      if (!IsInside(size, x, y)) {
        continue;
      }
      (*mask)[static_cast<std::size_t>(ToIndex(size, x, y))] = 1;
    }
  }
}

void MarkObjectInfluence(const RuntimeObject& object, const LevelSize& size,
                         std::vector<std::uint8_t>* mask) {
  if (mask == nullptr || object.width <= 0 || object.height <= 0) {
    return;
  }
  const int min_x = std::max(0, object.x - 3);
  const int min_y = std::max(0, object.y - 3);
  const int max_x = std::min(size.width - 1, object.x + object.width + 2);
  const int max_y = std::min(size.height - 1, object.y + object.height + 2);
  for (int y = min_y; y <= max_y; ++y) {
    for (int x = min_x; x <= max_x; ++x) {
      (*mask)[static_cast<std::size_t>(ToIndex(size, x, y))] = 1;
    }
  }
}

std::vector<std::uint8_t> BuildPlaceInfluence(const LevelData& level) {
  std::vector<std::uint8_t> influence(
      static_cast<std::size_t>(CellCount(level.size)), 0);
  for (const Place& place : level.places) {
    const int radius = std::clamp(place.radius > 0 ? place.radius : 4, 3, 12);
    MarkRadius(place.x, place.y, radius, level.size, &influence);
  }
  return influence;
}

std::vector<std::uint8_t> BuildObjectInfluence(const LevelData& level) {
  std::vector<std::uint8_t> influence(
      static_cast<std::size_t>(CellCount(level.size)), 0);
  for (const RuntimeObject& object : level.objects) {
    MarkObjectInfluence(object, level.size, &influence);
  }
  return influence;
}

std::vector<std::uint8_t> BuildRuinInfluence(const LevelData& level,
                                             const SemanticMasks& masks) {
  std::vector<std::uint8_t> influence(
      static_cast<std::size_t>(CellCount(level.size)), 0);
  for (int y = 0; y < level.size.height; ++y) {
    for (int x = 0; x < level.size.width; ++x) {
      const int index = ToIndex(level.size, x, y);
      const bool is_ruin = masks.ruins[static_cast<std::size_t>(index)] != 0 ||
                           masks.wall[static_cast<std::size_t>(index)] != 0;
      if (!is_ruin) {
        continue;
      }
      MarkRadius(x, y, 3, level.size, &influence);
    }
  }
  return influence;
}

std::vector<std::uint8_t> CombineSceneInfluence(
    const std::vector<std::uint8_t>& place_influence,
    const std::vector<std::uint8_t>& object_influence,
    const std::vector<std::uint8_t>& ruin_influence) {
  std::vector<std::uint8_t> influence(place_influence.size(), 0);
  for (std::size_t i = 0; i < influence.size(); ++i) {
    if (place_influence[i] != 0 || object_influence[i] != 0 ||
        ruin_influence[i] != 0) {
      influence[i] = 1;
    }
  }
  return influence;
}

std::vector<int> BuildForestDistance(const SemanticMasks& masks) {
  const int cell_count = CellCount(masks.size);
  constexpr int kMaxDistance = std::numeric_limits<int>::max() / 4;
  std::vector<int> distance(static_cast<std::size_t>(cell_count),
                            kMaxDistance);
  std::queue<int> queue;

  for (int index = 0; index < cell_count; ++index) {
    if (masks.forest[static_cast<std::size_t>(index)] == 0) {
      distance[static_cast<std::size_t>(index)] = 0;
      queue.push(index);
    }
  }

  if (queue.empty()) {
    std::fill(distance.begin(), distance.end(), kForestMidMaxDistance + 1);
    return distance;
  }

  constexpr std::array<int, 4> kDx{-1, 1, 0, 0};
  constexpr std::array<int, 4> kDy{0, 0, -1, 1};
  while (!queue.empty()) {
    const int current = queue.front();
    queue.pop();
    const int current_x = current % masks.size.width;
    const int current_y = current / masks.size.width;
    const int next_distance = distance[static_cast<std::size_t>(current)] + 1;

    for (std::size_t direction = 0; direction < kDx.size(); ++direction) {
      const int nx = current_x + kDx[direction];
      const int ny = current_y + kDy[direction];
      if (!IsInside(masks.size, nx, ny)) {
        continue;
      }
      const int neighbor = ToIndex(masks.size, nx, ny);
      if (masks.forest[static_cast<std::size_t>(neighbor)] == 0) {
        continue;
      }
      if (distance[static_cast<std::size_t>(neighbor)] <= next_distance) {
        continue;
      }
      distance[static_cast<std::size_t>(neighbor)] = next_distance;
      queue.push(neighbor);
    }
  }

  return distance;
}

void CountForestDepth(ForestDepthBand band, ForestVisualSummary* summary) {
  if (summary == nullptr) {
    return;
  }
  switch (band) {
    case ForestDepthBand::kEdge:
      ++summary->forest_edge_tiles;
      break;
    case ForestDepthBand::kMid:
      ++summary->forest_mid_tiles;
      break;
    case ForestDepthBand::kDeep:
      ++summary->forest_deep_tiles;
      break;
    case ForestDepthBand::kNone:
      break;
  }
}

void CountClearingRole(ClearingRole role, ForestVisualSummary* summary) {
  if (summary == nullptr) {
    return;
  }
  switch (role) {
    case ClearingRole::kMainClearing:
      ++summary->main_clearing_tiles;
      break;
    case ClearingRole::kSideClearing:
      ++summary->side_clearing_tiles;
      break;
    case ClearingRole::kConnectorCorridor:
      ++summary->connector_corridor_tiles;
      break;
    case ClearingRole::kMicroClearing:
      ++summary->micro_clearing_tiles;
      break;
    case ClearingRole::kSceneSpace:
      ++summary->scene_space_tiles;
      break;
    case ClearingRole::kNone:
      break;
  }
}

void CountClearingSceneRole(ClearingSceneRole role,
                            ForestVisualSummary* summary) {
  if (summary == nullptr) {
    return;
  }
  switch (role) {
    case ClearingSceneRole::kRuinsScene:
      ++summary->ruins_scene_tiles;
      break;
    case ClearingSceneRole::kRoadApproach:
      ++summary->road_approach_scene_tiles;
      break;
    case ClearingSceneRole::kObjectScene:
      ++summary->object_scene_tiles;
      break;
    case ClearingSceneRole::kGenericScene:
      ++summary->generic_scene_tiles;
      break;
    case ClearingSceneRole::kNone:
      break;
  }
}

ForestDepthBand ClassifyForestDepth(int distance) {
  if (distance <= 0) {
    return ForestDepthBand::kNone;
  }
  if (distance <= kForestEdgeMaxDistance) {
    return ForestDepthBand::kEdge;
  }
  if (distance <= kForestMidMaxDistance) {
    return ForestDepthBand::kMid;
  }
  return ForestDepthBand::kDeep;
}

ClearingRole ClassifyClearingRole(std::uint8_t route_influence,
                                  std::uint8_t scene_influence,
                                  int open_region_area) {
  if (scene_influence != 0) {
    return ClearingRole::kSceneSpace;
  }
  if (route_influence >= 3) {
    return ClearingRole::kMainClearing;
  }
  if (route_influence > 0) {
    return ClearingRole::kConnectorCorridor;
  }
  if (open_region_area > kNoRegionArea &&
      open_region_area <= kTinyOpenRegionArea) {
    return ClearingRole::kMicroClearing;
  }
  return ClearingRole::kSideClearing;
}

ClearingSceneRole ClassifyClearingSceneRole(std::uint8_t route_influence,
                                            std::uint8_t scene_influence,
                                            std::uint8_t ruin_influence,
                                            std::uint8_t object_influence,
                                            std::uint8_t place_influence) {
  if (scene_influence == 0) {
    return ClearingSceneRole::kNone;
  }
  if (route_influence != 0 &&
      (ruin_influence != 0 || object_influence != 0 || place_influence != 0)) {
    return ClearingSceneRole::kRoadApproach;
  }
  if (ruin_influence != 0) {
    return ClearingSceneRole::kRuinsScene;
  }
  if (object_influence != 0) {
    return ClearingSceneRole::kObjectScene;
  }
  return ClearingSceneRole::kGenericScene;
}

}  // namespace

const char* ForestDepthBandName(ForestDepthBand band) {
  switch (band) {
    case ForestDepthBand::kNone:
      return "none";
    case ForestDepthBand::kEdge:
      return "edge";
    case ForestDepthBand::kMid:
      return "mid";
    case ForestDepthBand::kDeep:
      return "deep";
  }
  return "none";
}

const char* ClearingRoleName(ClearingRole role) {
  switch (role) {
    case ClearingRole::kNone:
      return "none";
    case ClearingRole::kMainClearing:
      return "main_clearing";
    case ClearingRole::kSideClearing:
      return "side_clearing";
    case ClearingRole::kConnectorCorridor:
      return "connector_corridor";
    case ClearingRole::kMicroClearing:
      return "micro_clearing";
    case ClearingRole::kSceneSpace:
      return "scene_space";
  }
  return "none";
}

const char* ClearingSceneRoleName(ClearingSceneRole role) {
  switch (role) {
    case ClearingSceneRole::kNone:
      return "none";
    case ClearingSceneRole::kRuinsScene:
      return "ruins_scene";
    case ClearingSceneRole::kRoadApproach:
      return "road_approach";
    case ClearingSceneRole::kObjectScene:
      return "object_scene";
    case ClearingSceneRole::kGenericScene:
      return "generic_scene";
  }
  return "none";
}

std::string ForestVisualSummary::Dump() const {
  return "ForestVisualSummary { forest=" + std::to_string(forest_tiles) +
         ", edge=" + std::to_string(forest_edge_tiles) +
         ", mid=" + std::to_string(forest_mid_tiles) +
         ", deep=" + std::to_string(forest_deep_tiles) +
         ", suppressed_tiny=" +
         std::to_string(suppressed_tiny_forest_tiles) +
         ", canopy=" + std::to_string(canopy_candidate_tiles) +
         ", forest_groups=" + std::to_string(forest_mass_group_count) +
         ", route_influence=" + std::to_string(route_influenced_tiles) +
         ", main_clearing=" + std::to_string(main_clearing_tiles) +
         ", side_clearing=" + std::to_string(side_clearing_tiles) +
         ", connector=" + std::to_string(connector_corridor_tiles) +
         ", micro=" + std::to_string(micro_clearing_tiles) +
         ", scene=" + std::to_string(scene_space_tiles) +
         ", ruins_scene=" + std::to_string(ruins_scene_tiles) +
         ", road_approach=" + std::to_string(road_approach_scene_tiles) +
         ", object_scene=" + std::to_string(object_scene_tiles) +
         ", generic_scene=" + std::to_string(generic_scene_tiles) + " }";
}

bool ForestVisualPlan::IsValid() const {
  const int expected_size = CellCount(size);
  if (expected_size <= 0) {
    return false;
  }
  const std::size_t expected = static_cast<std::size_t>(expected_size);
  return forest_depth.size() == expected && forest_edges.size() == expected &&
         forest_mass_groups.size() == expected &&
         canopy_candidates.size() == expected &&
         clearing_roles.size() == expected &&
         clearing_scene_roles.size() == expected &&
         route_influence.size() == expected;
}

std::string ForestVisualPlan::Dump() const {
  return "ForestVisualPlan { size=" + std::to_string(size.width) + "x" +
         std::to_string(size.height) + ", " + summary.Dump() + " }";
}

ForestVisualPlan BuildForestVisualPlan(const LevelData& level,
                                       const SemanticMasks& masks,
                                       const TerrainRegions& regions,
                                       std::string* error) {
  ForestVisualPlan plan;
  plan.size = level.size;
  const int cell_count = CellCount(level.size);
  if (cell_count <= 0) {
    if (error != nullptr) {
      *error = "level size is invalid for forest visual planning";
    }
    return plan;
  }
  if (!masks.IsValid()) {
    if (error != nullptr) {
      *error = "semantic masks are invalid for forest visual planning";
    }
    return plan;
  }
  if (!regions.IsValid()) {
    if (error != nullptr) {
      *error = "terrain regions are invalid for forest visual planning";
    }
    return plan;
  }

  const std::size_t expected_size = static_cast<std::size_t>(cell_count);
  plan.forest_depth.assign(expected_size, 0);
  plan.forest_edges.assign(expected_size, 0);
  plan.forest_mass_groups.assign(expected_size, 0);
  plan.canopy_candidates.assign(expected_size, 0);
  plan.clearing_roles.assign(expected_size, 0);
  plan.clearing_scene_roles.assign(expected_size, 0);
  plan.route_influence = BuildRouteInfluence(level);

  std::vector<int> forest_region_area(expected_size, kNoRegionArea);
  BuildForestRegionAreaLookup(regions, &forest_region_area);
  plan.summary.forest_mass_group_count = BuildForestMassGroups(
      regions, level.size, &plan.forest_mass_groups);

  std::vector<int> open_region_area(expected_size, kNoRegionArea);
  BuildOpenRegionAreaLookup(regions, &open_region_area);
  const std::vector<std::uint8_t> place_influence =
      BuildPlaceInfluence(level);
  const std::vector<std::uint8_t> object_influence =
      BuildObjectInfluence(level);
  const std::vector<std::uint8_t> ruin_influence =
      BuildRuinInfluence(level, masks);
  const std::vector<std::uint8_t> scene_influence = CombineSceneInfluence(
      place_influence, object_influence, ruin_influence);
  const std::vector<int> forest_distance = BuildForestDistance(masks);

  for (int index = 0; index < cell_count; ++index) {
    const std::size_t item = static_cast<std::size_t>(index);
    if (plan.route_influence[item] != 0) {
      ++plan.summary.route_influenced_tiles;
    }

    if (masks.forest[item] != 0) {
      ++plan.summary.forest_tiles;
      if (forest_region_area[item] > kNoRegionArea &&
          forest_region_area[item] <= kTinyForestRegionArea) {
        ++plan.summary.suppressed_tiny_forest_tiles;
        continue;
      }

      const ForestDepthBand band = ClassifyForestDepth(forest_distance[item]);
      plan.forest_depth[item] = static_cast<std::uint8_t>(band);
      if (band == ForestDepthBand::kEdge) {
        plan.forest_edges[item] = 1;
      }
      if (band == ForestDepthBand::kMid || band == ForestDepthBand::kDeep) {
        plan.canopy_candidates[item] = 1;
        ++plan.summary.canopy_candidate_tiles;
      }
      CountForestDepth(band, &plan.summary);
      continue;
    }

    if (masks.open_ground[item] != 0) {
      const ClearingRole role = ClassifyClearingRole(
          plan.route_influence[item], scene_influence[item],
          open_region_area[item]);
      plan.clearing_roles[item] = static_cast<std::uint8_t>(role);
      CountClearingRole(role, &plan.summary);

      const ClearingSceneRole scene_role = ClassifyClearingSceneRole(
          plan.route_influence[item], scene_influence[item], ruin_influence[item],
          object_influence[item], place_influence[item]);
      plan.clearing_scene_roles[item] = static_cast<std::uint8_t>(scene_role);
      CountClearingSceneRole(scene_role, &plan.summary);
    }
  }

  return plan;
}

}  // namespace sar::visual_pipeline
