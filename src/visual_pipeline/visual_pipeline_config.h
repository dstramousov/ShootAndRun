#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_VISUAL_PIPELINE_CONFIG_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_VISUAL_PIPELINE_CONFIG_H_

#include <filesystem>
#include <optional>
#include <string_view>

namespace sar::visual_pipeline {

enum class VisualPipelineMode {
  kUsePreparedVisualMap,
  kBuildCpp,
  kHybrid,
  kCompare,
};

/**
 * @brief Returns the stable configuration name for a visual pipeline mode.
 *
 * @param mode Visual preparation mode.
 * @return Stable lowercase configuration value.
 */
const char* VisualPipelineModeName(VisualPipelineMode mode);

/**
 * @brief Parses a visual pipeline mode from a configuration string.
 *
 * @param value Configuration value.
 * @return Parsed mode when the value is supported.
 */
std::optional<VisualPipelineMode> ParseVisualPipelineMode(
    std::string_view value);

struct VisualPipelineConfig {
  VisualPipelineMode mode = VisualPipelineMode::kUsePreparedVisualMap;
  std::filesystem::path prepared_visual_map_path =
      "../visual_map/visual_map.json";
  bool fallback_to_cpp_pipeline = true;
  bool run_cpp_analysis = true;
  bool write_debug_artifacts = true;
  std::filesystem::path debug_output_path = "../prepared_map/debug";
};

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_VISUAL_PIPELINE_CONFIG_H_
