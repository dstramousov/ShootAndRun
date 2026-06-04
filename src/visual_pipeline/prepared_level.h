#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_PREPARED_LEVEL_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_PREPARED_LEVEL_H_

#include <string>

#include "level/level_data.h"

namespace sar::visual_pipeline {

struct PreparedLevel {
  bool ready = false;
  LevelSize size;
  int semantic_mask_count = 0;
  int terrain_region_count = 0;
  int visual_layer_count = 0;
  int decoration_count = 0;
  int render_cache_entry_count = 0;

  /**
   * @brief Returns a readable dump of the prepared level state.
   *
   * @return String representation for logs and debug overlays.
   */
  std::string Dump() const;
};

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_PREPARED_LEVEL_H_
