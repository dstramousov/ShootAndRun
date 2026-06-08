#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_VISUAL_MAP_LOADER_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_VISUAL_MAP_LOADER_H_

/**
 * @file src/visual_pipeline/visual_map_loader.h
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains public
 * declarations for visual_map_loader.h.
 */

#include <filesystem>
#include <string>

#include "level/level_data.h"
#include "visual_pipeline/visual_map_data.h"

namespace sar::visual_pipeline {

/**
 * @brief Stores visual map load result data shared between runtime systems.
 */
struct VisualMapLoadResult {
  bool ok = false;  ///< true when the operation completed successfully.
  bool found = false;  ///< true when the optional source file was present.
  VisualMapData data;  ///< Data value carried by this data structure.
  std::string error;  ///< Human-readable error message when loading or validation fails.
};

/**
 * @brief Owns the visual map loader behavior and its runtime state.
 */
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
