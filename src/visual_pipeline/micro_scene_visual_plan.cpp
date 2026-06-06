#include "visual_pipeline/micro_scene_visual_plan.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <sstream>
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

std::size_t ToItem(const LevelSize& size, int x, int y) {
  return static_cast<std::size_t>(ToIndex(size, x, y));
}

bool Contains(std::string_view text, std::string_view needle) {
  return text.find(needle) != std::string_view::npos;
}

std::uint32_t TileHash(int x, int y, std::uint32_t salt) {
  return (static_cast<std::uint32_t>(x) * 1103515245U) ^
         (static_cast<std::uint32_t>(y) * 2654435761U) ^ salt;
}

std::uint8_t TilePriority(MicroSceneTile tile) {
  switch (tile) {
    case MicroSceneTile::kPrimaryProp:
      return 8;
    case MicroSceneTile::kSecondaryProp:
      return 7;
    case MicroSceneTile::kStoneDetail:
      return 6;
    case MicroSceneTile::kWetDetail:
      return 5;
    case MicroSceneTile::kSmallDebris:
      return 4;
    case MicroSceneTile::kVegetationDetail:
      return 3;
    case MicroSceneTile::kGroundDetail:
      return 2;
    case MicroSceneTile::kNone:
      return 0;
  }
  return 0;
}

bool IsBlockingForDressing(const SemanticMasks& masks, int x, int y) {
  if (!IsInside(masks.size, x, y)) {
    return true;
  }
  const std::size_t item = ToItem(masks.size, x, y);
  return masks.blocked[item] != 0 && masks.ruins[item] == 0 &&
         masks.wall[item] == 0;
}

bool IsWaterCoreTile(const WaterVisualPlan& plan, int x, int y) {
  if (!plan.IsValid() || !IsInside(plan.size, x, y)) {
    return false;
  }
  const auto tile = static_cast<WaterVisualTile>(plan.tiles[ToItem(plan.size,
                                                                  x, y)]);
  return tile == WaterVisualTile::kWaterCore;
}

bool IsAllowedForScene(const SemanticMasks& masks,
                       const WaterVisualPlan& water_plan,
                       MicroSceneKind kind,
                       int x,
                       int y) {
  if (!IsInside(masks.size, x, y)) {
    return false;
  }
  if (IsWaterCoreTile(water_plan, x, y) &&
      kind != MicroSceneKind::kSwampCrossingDetail) {
    return false;
  }
  if (kind != MicroSceneKind::kRuinDebrisCluster &&
      IsBlockingForDressing(masks, x, y)) {
    return false;
  }
  return true;
}

void SetSceneTile(const LevelSize& size,
                  int x,
                  int y,
                  MicroSceneTile tile,
                  std::uint16_t scene_id,
                  MicroSceneVisualPlan* plan) {
  if (plan == nullptr || !IsInside(size, x, y)) {
    return;
  }
  const std::size_t item = ToItem(size, x, y);
  const auto current = static_cast<MicroSceneTile>(plan->tiles[item]);
  if (TilePriority(tile) >= TilePriority(current)) {
    plan->tiles[item] = static_cast<std::uint8_t>(tile);
    plan->scene_ids[item] = scene_id;
  }
}

MicroSceneKind KindForObject(const ObjectVisualItem& item) {
  if (item.kind == ObjectVisualKind::kCamp ||
      Contains(item.source_type, "camp") || Contains(item.source_type, "tent") ||
      Contains(item.source_type, "backpack")) {
    return MicroSceneKind::kCampScene;
  }
  if (item.kind == ObjectVisualKind::kCache ||
      Contains(item.source_type, "cache")) {
    return MicroSceneKind::kCacheHint;
  }
  if (item.kind == ObjectVisualKind::kWood ||
      Contains(item.source_type, "log") || Contains(item.source_type, "tree")) {
    return MicroSceneKind::kLoggingSpot;
  }
  if (item.kind == ObjectVisualKind::kRuin ||
      item.kind == ObjectVisualKind::kStructure ||
      item.kind == ObjectVisualKind::kStone ||
      item.kind == ObjectVisualKind::kScrap) {
    return MicroSceneKind::kObjectSceneDressing;
  }
  return MicroSceneKind::kObjectSceneDressing;
}

std::string ThemeForKind(MicroSceneKind kind) {
  switch (kind) {
    case MicroSceneKind::kCampScene:
      return "camp_scene";
    case MicroSceneKind::kRoadsideDebris:
      return "roadside_debris";
    case MicroSceneKind::kLoggingSpot:
      return "logging_spot";
    case MicroSceneKind::kRuinDebrisCluster:
      return "ruin_debris_cluster";
    case MicroSceneKind::kSwampCrossingDetail:
      return "swamp_crossing_detail";
    case MicroSceneKind::kObjectSceneDressing:
      return "object_scene_dressing";
    case MicroSceneKind::kCacheHint:
      return "cache_hint";
    case MicroSceneKind::kNone:
      return "none";
  }
  return "none";
}

std::string PrimaryPropForKind(MicroSceneKind kind) {
  switch (kind) {
    case MicroSceneKind::kCampScene:
      return "dead_campfire_or_tent";
    case MicroSceneKind::kRoadsideDebris:
      return "stone_or_branch";
    case MicroSceneKind::kLoggingSpot:
      return "fallen_log_or_stump";
    case MicroSceneKind::kRuinDebrisCluster:
      return "stone_rubble";
    case MicroSceneKind::kSwampCrossingDetail:
      return "reeds_or_wet_grass";
    case MicroSceneKind::kObjectSceneDressing:
      return "context_prop";
    case MicroSceneKind::kCacheHint:
      return "cache_marker";
    case MicroSceneKind::kNone:
      return "none";
  }
  return "none";
}

int PriorityForKind(MicroSceneKind kind) {
  switch (kind) {
    case MicroSceneKind::kCacheHint:
      return 80;
    case MicroSceneKind::kCampScene:
      return 75;
    case MicroSceneKind::kRuinDebrisCluster:
      return 65;
    case MicroSceneKind::kSwampCrossingDetail:
      return 60;
    case MicroSceneKind::kLoggingSpot:
      return 55;
    case MicroSceneKind::kRoadsideDebris:
      return 45;
    case MicroSceneKind::kObjectSceneDressing:
      return 40;
    case MicroSceneKind::kNone:
      return 0;
  }
  return 0;
}

int RadiusForScene(MicroSceneKind kind, const ObjectVisualItem* item) {
  if (item != nullptr && (item->width > 1 || item->height > 1)) {
    return 2;
  }
  switch (kind) {
    case MicroSceneKind::kCampScene:
      return 3;
    case MicroSceneKind::kRuinDebrisCluster:
      return 2;
    case MicroSceneKind::kSwampCrossingDetail:
      return 2;
    case MicroSceneKind::kLoggingSpot:
      return 2;
    case MicroSceneKind::kRoadsideDebris:
      return 1;
    case MicroSceneKind::kObjectSceneDressing:
      return 1;
    case MicroSceneKind::kCacheHint:
      return 2;
    case MicroSceneKind::kNone:
      return 1;
  }
  return 1;
}

void IncrementKindCount(MicroSceneKind kind, MicroSceneSummary* summary) {
  if (summary == nullptr) {
    return;
  }
  switch (kind) {
    case MicroSceneKind::kCampScene:
      ++summary->camp_scene_count;
      break;
    case MicroSceneKind::kRoadsideDebris:
      ++summary->roadside_debris_count;
      break;
    case MicroSceneKind::kLoggingSpot:
      ++summary->logging_spot_count;
      break;
    case MicroSceneKind::kRuinDebrisCluster:
      ++summary->ruin_debris_cluster_count;
      break;
    case MicroSceneKind::kSwampCrossingDetail:
      ++summary->swamp_crossing_detail_count;
      break;
    case MicroSceneKind::kObjectSceneDressing:
      ++summary->object_scene_dressing_count;
      break;
    case MicroSceneKind::kCacheHint:
      ++summary->cache_hint_count;
      break;
    case MicroSceneKind::kNone:
      break;
  }
}

MicroSceneTile SecondaryTileForKind(MicroSceneKind kind) {
  switch (kind) {
    case MicroSceneKind::kCampScene:
      return MicroSceneTile::kSecondaryProp;
    case MicroSceneKind::kRoadsideDebris:
      return MicroSceneTile::kGroundDetail;
    case MicroSceneKind::kLoggingSpot:
      return MicroSceneTile::kVegetationDetail;
    case MicroSceneKind::kRuinDebrisCluster:
      return MicroSceneTile::kStoneDetail;
    case MicroSceneKind::kSwampCrossingDetail:
      return MicroSceneTile::kWetDetail;
    case MicroSceneKind::kObjectSceneDressing:
      return MicroSceneTile::kSmallDebris;
    case MicroSceneKind::kCacheHint:
      return MicroSceneTile::kStoneDetail;
    case MicroSceneKind::kNone:
      return MicroSceneTile::kGroundDetail;
  }
  return MicroSceneTile::kGroundDetail;
}

MicroSceneTile OuterTileForKind(MicroSceneKind kind) {
  switch (kind) {
    case MicroSceneKind::kSwampCrossingDetail:
      return MicroSceneTile::kWetDetail;
    case MicroSceneKind::kLoggingSpot:
      return MicroSceneTile::kVegetationDetail;
    case MicroSceneKind::kRuinDebrisCluster:
      return MicroSceneTile::kStoneDetail;
    case MicroSceneKind::kCampScene:
    case MicroSceneKind::kRoadsideDebris:
    case MicroSceneKind::kObjectSceneDressing:
    case MicroSceneKind::kCacheHint:
    case MicroSceneKind::kNone:
      return MicroSceneTile::kSmallDebris;
  }
  return MicroSceneTile::kSmallDebris;
}

void PaintSceneFootprint(const SemanticMasks& masks,
                         const WaterVisualPlan& water_plan,
                         const MicroSceneItem& scene,
                         std::uint16_t scene_id,
                         MicroSceneVisualPlan* plan) {
  if (plan == nullptr) {
    return;
  }
  for (int dy = -scene.radius; dy <= scene.radius; ++dy) {
    for (int dx = -scene.radius; dx <= scene.radius; ++dx) {
      const int x = scene.x + dx;
      const int y = scene.y + dy;
      if (!IsAllowedForScene(masks, water_plan, scene.kind, x, y)) {
        continue;
      }
      const int distance_squared = dx * dx + dy * dy;
      if (distance_squared > scene.radius * scene.radius) {
        continue;
      }
      if (dx == 0 && dy == 0) {
        SetSceneTile(plan->size, x, y, MicroSceneTile::kPrimaryProp, scene_id,
                     plan);
      } else if (distance_squared <= 2) {
        SetSceneTile(plan->size, x, y, SecondaryTileForKind(scene.kind),
                     scene_id, plan);
      } else if (TileHash(x, y, static_cast<std::uint32_t>(scene.priority)) %
                     3U !=
                 0U) {
        SetSceneTile(plan->size, x, y, OuterTileForKind(scene.kind), scene_id,
                     plan);
      }
    }
  }
}

void AddScene(MicroSceneKind kind,
              int x,
              int y,
              int radius,
              MicroSceneVisualPlan* plan) {
  if (plan == nullptr || !IsInside(plan->size, x, y)) {
    return;
  }

  MicroSceneItem scene;
  scene.id = "scene_" + std::to_string(plan->scenes.size() + 1);
  scene.kind = kind;
  scene.theme = ThemeForKind(kind);
  scene.primary_prop = PrimaryPropForKind(kind);
  scene.x = x;
  scene.y = y;
  scene.radius = radius;
  scene.priority = PriorityForKind(kind);
  plan->scenes.push_back(std::move(scene));
  ++plan->summary.theme_counts[ThemeForKind(kind)];
  IncrementKindCount(kind, &plan->summary);
}

void AddObjectScenes(const ObjectVisualPlan& object_plan,
                     MicroSceneVisualPlan* plan) {
  if (plan == nullptr) {
    return;
  }

  for (const ObjectVisualItem& item : object_plan.items) {
    const MicroSceneKind kind = KindForObject(item);
    const int center_x = item.x + std::max(item.width - 1, 0) / 2;
    const int center_y = item.y + std::max(item.height - 1, 0) / 2;
    AddScene(kind, center_x, center_y, RadiusForScene(kind, &item), plan);
  }
}

void AddRoadsideScenes(const RoadVisualPlan& road_plan,
                       const SemanticMasks& masks,
                       MicroSceneVisualPlan* plan) {
  if (plan == nullptr || !road_plan.IsValid()) {
    return;
  }

  int added = 0;
  constexpr int kMaxRoadsideScenes = 24;
  for (int y = 0; y < road_plan.size.height && added < kMaxRoadsideScenes; ++y) {
    for (int x = 0; x < road_plan.size.width && added < kMaxRoadsideScenes; ++x) {
      const std::size_t item = ToItem(road_plan.size, x, y);
      const auto band = static_cast<RoadVisualBand>(road_plan.road_bands[item]);
      if (band != RoadVisualBand::kRoadSide &&
          band != RoadVisualBand::kTrampledGrass) {
        continue;
      }
      if (masks.blocked[item] != 0 || TileHash(x, y, 0x524F4144U) % 17U != 0U) {
        continue;
      }
      AddScene(MicroSceneKind::kRoadsideDebris, x, y, 1, plan);
      ++added;
    }
  }
}

void AddRuinScenes(const RuinVisualPlan& ruin_plan,
                   const SemanticMasks& masks,
                   MicroSceneVisualPlan* plan) {
  if (plan == nullptr || !ruin_plan.IsValid()) {
    return;
  }

  int added = 0;
  constexpr int kMaxRuinScenes = 32;
  for (int y = 0; y < ruin_plan.size.height && added < kMaxRuinScenes; ++y) {
    for (int x = 0; x < ruin_plan.size.width && added < kMaxRuinScenes; ++x) {
      const std::size_t item = ToItem(ruin_plan.size, x, y);
      const auto role = static_cast<RuinVisualTile>(ruin_plan.tiles[item]);
      if (role != RuinVisualTile::kRubble &&
          role != RuinVisualTile::kCrackedFloor &&
          role != RuinVisualTile::kWallBroken) {
        continue;
      }
      if (masks.walkable[item] == 0 && role == RuinVisualTile::kCrackedFloor) {
        continue;
      }
      if (TileHash(x, y, 0x5255494EU) % 23U != 0U) {
        continue;
      }
      AddScene(MicroSceneKind::kRuinDebrisCluster, x, y, 2, plan);
      ++added;
    }
  }
}

void AddSwampScenes(const WaterVisualPlan& water_plan,
                    MicroSceneVisualPlan* plan) {
  if (plan == nullptr || !water_plan.IsValid()) {
    return;
  }

  int added = 0;
  constexpr int kMaxSwampScenes = 24;
  for (int y = 0; y < water_plan.size.height && added < kMaxSwampScenes; ++y) {
    for (int x = 0; x < water_plan.size.width && added < kMaxSwampScenes; ++x) {
      const std::size_t item = ToItem(water_plan.size, x, y);
      const auto role = static_cast<WaterVisualTile>(water_plan.tiles[item]);
      if (role != WaterVisualTile::kCrossing &&
          role != WaterVisualTile::kReedZone &&
          role != WaterVisualTile::kMudRing) {
        continue;
      }
      if (TileHash(x, y, 0x57544554U) % 19U != 0U) {
        continue;
      }
      AddScene(MicroSceneKind::kSwampCrossingDetail, x, y, 2, plan);
      ++added;
    }
  }
}

void CountTile(MicroSceneTile tile, MicroSceneSummary* summary) {
  if (summary == nullptr) {
    return;
  }
  switch (tile) {
    case MicroSceneTile::kPrimaryProp:
      ++summary->primary_prop_tiles;
      break;
    case MicroSceneTile::kSecondaryProp:
      ++summary->secondary_prop_tiles;
      break;
    case MicroSceneTile::kSmallDebris:
      ++summary->small_debris_tiles;
      break;
    case MicroSceneTile::kGroundDetail:
      ++summary->ground_detail_tiles;
      break;
    case MicroSceneTile::kVegetationDetail:
      ++summary->vegetation_detail_tiles;
      break;
    case MicroSceneTile::kStoneDetail:
      ++summary->stone_detail_tiles;
      break;
    case MicroSceneTile::kWetDetail:
      ++summary->wet_detail_tiles;
      break;
    case MicroSceneTile::kNone:
      break;
  }
}

void FinalizeSummary(MicroSceneVisualPlan* plan) {
  if (plan == nullptr) {
    return;
  }
  plan->summary.scene_count = static_cast<int>(plan->scenes.size());
  for (const std::uint8_t value : plan->tiles) {
    const auto tile = static_cast<MicroSceneTile>(value);
    if (tile == MicroSceneTile::kNone) {
      continue;
    }
    ++plan->summary.visual_tiles;
    CountTile(tile, &plan->summary);
  }
}

}  // namespace

const char* MicroSceneKindName(MicroSceneKind kind) {
  switch (kind) {
    case MicroSceneKind::kCampScene:
      return "camp_scene";
    case MicroSceneKind::kRoadsideDebris:
      return "roadside_debris";
    case MicroSceneKind::kLoggingSpot:
      return "logging_spot";
    case MicroSceneKind::kRuinDebrisCluster:
      return "ruin_debris_cluster";
    case MicroSceneKind::kSwampCrossingDetail:
      return "swamp_crossing_detail";
    case MicroSceneKind::kObjectSceneDressing:
      return "object_scene_dressing";
    case MicroSceneKind::kCacheHint:
      return "cache_hint";
    case MicroSceneKind::kNone:
      return "none";
  }
  return "none";
}

const char* MicroSceneTileName(MicroSceneTile tile) {
  switch (tile) {
    case MicroSceneTile::kGroundDetail:
      return "ground_detail";
    case MicroSceneTile::kSmallDebris:
      return "small_debris";
    case MicroSceneTile::kSecondaryProp:
      return "secondary_prop";
    case MicroSceneTile::kPrimaryProp:
      return "primary_prop";
    case MicroSceneTile::kVegetationDetail:
      return "vegetation_detail";
    case MicroSceneTile::kStoneDetail:
      return "stone_detail";
    case MicroSceneTile::kWetDetail:
      return "wet_detail";
    case MicroSceneTile::kNone:
      return "none";
  }
  return "none";
}

std::string MicroSceneSummary::Dump() const {
  std::ostringstream stream;
  stream << "MicroSceneSummary { scenes: " << scene_count
         << ", camp: " << camp_scene_count
         << ", roadside: " << roadside_debris_count
         << ", logging: " << logging_spot_count
         << ", ruins: " << ruin_debris_cluster_count
         << ", swamp: " << swamp_crossing_detail_count
         << ", object: " << object_scene_dressing_count
         << ", cache: " << cache_hint_count
         << ", visual_tiles: " << visual_tiles << " }";
  return stream.str();
}

bool MicroSceneVisualPlan::IsValid() const {
  const int expected_size = size.width * size.height;
  return size.width > 0 && size.height > 0 && size.tile_size > 0 &&
         expected_size > 0 &&
         tiles.size() == static_cast<std::size_t>(expected_size) &&
         scene_ids.size() == static_cast<std::size_t>(expected_size);
}

std::string MicroSceneVisualPlan::Dump() const {
  std::ostringstream stream;
  stream << "MicroSceneVisualPlan { size: " << size.width << 'x'
         << size.height << ", scenes: " << scenes.size()
         << ", summary: " << summary.Dump() << " }";
  return stream.str();
}

MicroSceneVisualPlan BuildMicroSceneVisualPlan(
    const LevelData& level,
    const SemanticMasks& masks,
    const ForestVisualPlan& forest_plan,
    const RoadVisualPlan& road_plan,
    const RuinVisualPlan& ruin_plan,
    const WaterVisualPlan& water_plan,
    const ObjectVisualPlan& object_plan,
    std::string* error) {
  (void)forest_plan;
  MicroSceneVisualPlan plan;
  plan.size = level.size;

  const int cell_count = CellCount(level.size);
  if (cell_count <= 0 || masks.size.width != level.size.width ||
      masks.size.height != level.size.height || !object_plan.IsValid()) {
    if (error != nullptr) {
      *error = "invalid input for micro-scene visual plan";
    }
    return {};
  }

  plan.tiles.assign(static_cast<std::size_t>(cell_count),
                    static_cast<std::uint8_t>(MicroSceneTile::kNone));
  plan.scene_ids.assign(static_cast<std::size_t>(cell_count), 0);

  AddObjectScenes(object_plan, &plan);
  AddRoadsideScenes(road_plan, masks, &plan);
  AddRuinScenes(ruin_plan, masks, &plan);
  AddSwampScenes(water_plan, &plan);

  for (std::size_t i = 0; i < plan.scenes.size(); ++i) {
    const std::uint16_t scene_id = static_cast<std::uint16_t>(
        std::min<std::size_t>(i + 1, UINT16_MAX));
    PaintSceneFootprint(masks, water_plan, plan.scenes[i], scene_id, &plan);
  }

  FinalizeSummary(&plan);
  return plan;
}

}  // namespace sar::visual_pipeline
