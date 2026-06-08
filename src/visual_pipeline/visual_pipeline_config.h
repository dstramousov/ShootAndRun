#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_VISUAL_PIPELINE_CONFIG_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_VISUAL_PIPELINE_CONFIG_H_

/**
 * @file src/visual_pipeline/visual_pipeline_config.h
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains public
 * declarations for visual_pipeline_config.h.
 */

#include <filesystem>
#include <optional>
#include <string_view>

namespace sar::visual_pipeline {

/**
 * @brief Defines the supported visual pipeline mode values.
 */
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

/**
 * @brief Stores visual pipeline config data shared between runtime systems.
 */
struct VisualPipelineConfig {
  VisualPipelineMode mode = VisualPipelineMode::kUsePreparedVisualMap;  ///< Mode value carried by this data structure.
  std::filesystem::path prepared_visual_map_path =
      "../visual_map/visual_map.json";  ///< Json value carried by this data structure.
  bool fallback_to_cpp_pipeline = true;  ///< Fallback to c++ pipeline value carried by this data structure.
  bool run_cpp_analysis = true;  ///< Run c++ analysis value carried by this data structure.
  bool write_debug_artifacts = true;  ///< Write debug artifacts value carried by this data structure.
  std::filesystem::path debug_output_path = "../prepared_map/debug";  ///< Filesystem path used by this configuration or data object.
};

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_VISUAL_PIPELINE_CONFIG_H_
