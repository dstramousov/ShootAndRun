#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_VISUAL_MAP_LOADER_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_VISUAL_MAP_LOADER_H_

#include <filesystem>
#include <string>

#include "level/level_data.h"
#include "visual_pipeline/visual_map_data.h"

namespace sar::visual_pipeline {

struct VisualMapLoadResult {
  bool ok = false;
  bool found = false;
  VisualMapData data;
  std::string error;
};

class VisualMapLoader {
 public:
  /**
   * @brief Loads a prepared visual map manifest and its companion summaries.
   *
   * Missing files are reported as `found=false` so callers can decide whether
   * to fall back to the local C++ visual pipeline.
   *
   * @param manifest_path Path to `visual_map.json`.
   * @param raw_size Expected raw level dimensions.
   * @return Load result with either prepared visual data or an error message.
   */
  VisualMapLoadResult Load(const std::filesystem::path& manifest_path,
                           const LevelSize& raw_size) const;
};

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_VISUAL_MAP_LOADER_H_
