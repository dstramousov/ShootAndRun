#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_PREPARED_LEVEL_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_PREPARED_LEVEL_H_

#include <string>

#include "level/level_data.h"
#include "visual_pipeline/forest_visual_plan.h"
#include "visual_pipeline/road_visual_plan.h"
#include "visual_pipeline/semantic_masks.h"
#include "visual_pipeline/region_borders.h"
#include "visual_pipeline/terrain_regions.h"
#include "visual_pipeline/visual_map_data.h"

namespace sar::visual_pipeline {

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

struct PreparedLevel {
  bool ready = false;
  LevelSize size;
  PreparedLevelSource source = PreparedLevelSource::kCppPipeline;
  int semantic_mask_count = 0;
  SemanticMasks semantic_masks;
  int terrain_region_count = 0;
  TerrainRegions terrain_regions;
  int region_border_count = 0;
  RegionBorders region_borders;
  ForestVisualPlan forest_visual_plan;
  RoadVisualPlan road_visual_plan;
  int visual_layer_count = 0;
  int decoration_count = 0;
  int render_cache_entry_count = 0;
  VisualMapData prepared_visual_map;

  /**
   * @brief Returns a readable dump of the prepared level state.
   *
   * @return String representation for logs and debug overlays.
   */
  std::string Dump() const;
};

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_PREPARED_LEVEL_H_
