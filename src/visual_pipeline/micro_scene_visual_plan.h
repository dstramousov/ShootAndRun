#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_MICRO_SCENE_VISUAL_PLAN_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_MICRO_SCENE_VISUAL_PLAN_H_

/**
 * @file src/visual_pipeline/micro_scene_visual_plan.h
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains public
 * declarations for micro_scene_visual_plan.h.
 */

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

/**
 * @brief Type of a generated micro-scene.
 */
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

/**
 * @brief Visual role of one tile inside a generated micro-scene.
 */
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

/**
 * @brief One generated micro-scene placement.
 */
struct MicroSceneItem {
  std::string id;  ///< Stable identifier loaded from source data or configuration.
  MicroSceneKind kind = MicroSceneKind::kNone;  ///< Kind value carried by this data structure.
  std::string theme;  ///< Theme value carried by this data structure.
  std::string primary_prop;  ///< Primary prop value carried by this data structure.
  int x = 0;  ///< Tile, screen, or world coordinate for x.
  int y = 0;  ///< Tile, screen, or world coordinate for y.
  int radius = 1;  ///< Size component for radius.
  int priority = 0;  ///< Priority value carried by this data structure.
};

/**
 * @brief Counters produced by the micro-scene planning pass.
 */
struct MicroSceneSummary {
  int scene_count = 0;  ///< Count of scene count entries or events.
  int camp_scene_count = 0;  ///< Count of camp scene count entries or events.
  int roadside_debris_count = 0;  ///< Count of roadside debris count entries or events.
  int logging_spot_count = 0;  ///< Count of logging spot count entries or events.
  int ruin_debris_cluster_count = 0;  ///< Count of ruin debris cluster count entries or events.
  int swamp_crossing_detail_count = 0;  ///< Count of swamp crossing detail count entries or events.
  int object_scene_dressing_count = 0;  ///< Count of object scene dressing count entries or events.
  int cache_hint_count = 0;  ///< Count of cache hint count entries or events.
  int visual_tiles = 0;  ///< Visual tiles value carried by this data structure.
  int primary_prop_tiles = 0;  ///< Primary prop tiles value carried by this data structure.
  int secondary_prop_tiles = 0;  ///< Secondary prop tiles value carried by this data structure.
  int small_debris_tiles = 0;  ///< Small debris tiles value carried by this data structure.
  int ground_detail_tiles = 0;  ///< Ground detail tiles value carried by this data structure.
  int vegetation_detail_tiles = 0;  ///< Vegetation detail tiles value carried by this data structure.
  int stone_detail_tiles = 0;  ///< Stone detail tiles value carried by this data structure.
  int wet_detail_tiles = 0;  ///< Wet detail tiles value carried by this data structure.
  std::map<std::string, int> theme_counts;  ///< Count of theme counts entries or events.

  /**
   * @brief Returns a readable dump of micro-scene counters.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

/**
 * @brief Generated micro-scenes and their tile masks.
 */
struct MicroSceneVisualPlan {
  LevelSize size;  ///< Size value carried by this data structure.
  std::vector<std::uint8_t> tiles;  ///< Tiles value carried by this data structure.
  std::vector<std::uint16_t> scene_ids;  ///< Scene ids value carried by this data structure.
  std::vector<MicroSceneItem> scenes;  ///< Scenes value carried by this data structure.
  MicroSceneSummary summary;  ///< Summary value carried by this data structure.

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
