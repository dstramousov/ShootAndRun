#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_PREPARED_LEVEL_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_PREPARED_LEVEL_H_

/**
 * @file src/visual_pipeline/prepared_level.h
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains public
 * declarations for prepared_level.h.
 */

#include <filesystem>
#include <string>

#include "level/level_data.h"
#include "visual_pipeline/forest_visual_plan.h"
#include "visual_pipeline/object_visual_plan.h"
#include "visual_pipeline/micro_scene_visual_plan.h"
#include "visual_pipeline/road_visual_plan.h"
#include "visual_pipeline/ruin_visual_plan.h"
#include "visual_pipeline/water_visual_plan.h"
#include "visual_pipeline/semantic_masks.h"
#include "visual_pipeline/region_borders.h"
#include "visual_pipeline/terrain_regions.h"
#include "visual_pipeline/visual_map_data.h"

namespace sar::visual_pipeline {

/**
 * @brief Defines the supported prepared level source values.
 */
enum class PreparedLevelSource {
  kCppPipeline,
  kPreparedVisualMap,
  kHybrid,
};

/**
 * @brief Returns the stable name for a prepared-level source.
 *
 * @param source Prepared level source kind.
 * @return Stable lowercase source name.
 */
const char* PreparedLevelSourceName(PreparedLevelSource source);

/**
 * @brief Stores prepared level data shared between runtime systems.
 */
struct PreparedLevel {
  bool ready = false;  ///< Ready value carried by this data structure.
  LevelSize size;  ///< Size value carried by this data structure.
  PreparedLevelSource source = PreparedLevelSource::kCppPipeline;  ///< Source value carried by this data structure.
  int semantic_mask_count = 0;  ///< Count of semantic mask count entries or events.
  SemanticMasks semantic_masks;  ///< Semantic masks value carried by this data structure.
  int terrain_region_count = 0;  ///< Count of terrain region count entries or events.
  TerrainRegions terrain_regions;  ///< Terrain regions value carried by this data structure.
  int region_border_count = 0;  ///< Count of region border count entries or events.
  RegionBorders region_borders;  ///< Region borders value carried by this data structure.
  ForestVisualPlan forest_visual_plan;  ///< Forest visual plan value carried by this data structure.
  RoadVisualPlan road_visual_plan;  ///< Road visual plan value carried by this data structure.
  RuinVisualPlan ruin_visual_plan;  ///< Ruin visual plan value carried by this data structure.
  WaterVisualPlan water_visual_plan;  ///< Water visual plan value carried by this data structure.
  ObjectVisualPlan object_visual_plan;  ///< Object visual plan value carried by this data structure.
  MicroSceneVisualPlan micro_scene_visual_plan;  ///< Micro scene visual plan value carried by this data structure.
  int visual_layer_count = 0;  ///< Count of visual layer count entries or events.
  int decoration_count = 0;  ///< Count of decoration count entries or events.
  int render_cache_entry_count = 0;  ///< Count of render cache entry count entries or events.
  VisualMapData prepared_visual_map;  ///< Prepared visual map value carried by this data structure.
  std::filesystem::path visual_package_path;  ///< Filesystem path used by this configuration or data object.
  std::filesystem::path final_render_path;  ///< Filesystem path used by this configuration or data object.

  /**
   * @brief Returns a readable dump of the prepared level state.
   *
   * @return String representation for logs and debug overlays.
   */
  std::string Dump() const;
};

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_PREPARED_LEVEL_H_
