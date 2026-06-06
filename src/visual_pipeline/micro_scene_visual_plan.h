#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_MICRO_SCENE_VISUAL_PLAN_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_MICRO_SCENE_VISUAL_PLAN_H_

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "level/level_data.h"
#include "visual_pipeline/forest_visual_plan.h"
#include "visual_pipeline/object_visual_plan.h"
#include "visual_pipeline/road_visual_plan.h"
#include "visual_pipeline/ruin_visual_plan.h"
#include "visual_pipeline/semantic_masks.h"
#include "visual_pipeline/water_visual_plan.h"

namespace sar::visual_pipeline {

enum class MicroSceneKind : std::uint8_t {
  kNone = 0,
  kCampScene = 1,
  kRoadsideDebris = 2,
  kLoggingSpot = 3,
  kRuinDebrisCluster = 4,
  kSwampCrossingDetail = 5,
  kObjectSceneDressing = 6,
  kCacheHint = 7,
};

enum class MicroSceneTile : std::uint8_t {
  kNone = 0,
  kGroundDetail = 1,
  kSmallDebris = 2,
  kSecondaryProp = 3,
  kPrimaryProp = 4,
  kVegetationDetail = 5,
  kStoneDetail = 6,
  kWetDetail = 7,
};

/**
 * @brief Returns the stable name for a micro-scene kind.
 *
 * @param kind Micro-scene kind value.
 * @return Stable lowercase kind name.
 */
const char* MicroSceneKindName(MicroSceneKind kind);

/**
 * @brief Returns the stable name for a micro-scene tile role.
 *
 * @param tile Micro-scene tile role value.
 * @return Stable lowercase tile role name.
 */
const char* MicroSceneTileName(MicroSceneTile tile);

struct MicroSceneItem {
  std::string id;
  MicroSceneKind kind = MicroSceneKind::kNone;
  std::string theme;
  std::string primary_prop;
  int x = 0;
  int y = 0;
  int radius = 1;
  int priority = 0;
};

struct MicroSceneSummary {
  int scene_count = 0;
  int camp_scene_count = 0;
  int roadside_debris_count = 0;
  int logging_spot_count = 0;
  int ruin_debris_cluster_count = 0;
  int swamp_crossing_detail_count = 0;
  int object_scene_dressing_count = 0;
  int cache_hint_count = 0;
  int visual_tiles = 0;
  int primary_prop_tiles = 0;
  int secondary_prop_tiles = 0;
  int small_debris_tiles = 0;
  int ground_detail_tiles = 0;
  int vegetation_detail_tiles = 0;
  int stone_detail_tiles = 0;
  int wet_detail_tiles = 0;
  std::map<std::string, int> theme_counts;

  /**
   * @brief Returns a readable dump of micro-scene counters.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

struct MicroSceneVisualPlan {
  LevelSize size;
  std::vector<std::uint8_t> tiles;
  std::vector<std::uint16_t> scene_ids;
  std::vector<MicroSceneItem> scenes;
  MicroSceneSummary summary;

  /**
   * @brief Returns true when the plan matches the level dimensions.
   *
   * @return Validation flag.
   */
  bool IsValid() const;

  /**
   * @brief Returns a readable dump of the micro-scene visual plan.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

/**
 * @brief Builds visual-only micro-scene and dressing placement data.
 *
 * The pass keeps gameplay data unchanged. It groups existing object, road,
 * ruin and water context into small visual story spots instead of sprinkling
 * decoration evenly across the whole map.
 *
 * @param level Loaded level data.
 * @param masks Semantic masks produced from the raw level.
 * @param forest_plan Forest and clearing visual intent.
 * @param road_plan Road dressing visual intent.
 * @param ruin_plan Ruin scene visual intent.
 * @param water_plan Water and swamp visual intent.
 * @param object_plan Typed runtime-object visual mapping.
 * @param error Error text populated when building fails.
 * @return Built micro-scene plan. Returned data is invalid on failure.
 */
MicroSceneVisualPlan BuildMicroSceneVisualPlan(
    const LevelData& level,
    const SemanticMasks& masks,
    const ForestVisualPlan& forest_plan,
    const RoadVisualPlan& road_plan,
    const RuinVisualPlan& ruin_plan,
    const WaterVisualPlan& water_plan,
    const ObjectVisualPlan& object_plan,
    std::string* error);

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_MICRO_SCENE_VISUAL_PLAN_H_
