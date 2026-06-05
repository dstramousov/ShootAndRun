#include "visual_pipeline/visual_pipeline_config.h"

#include <optional>
#include <string_view>

namespace sar::visual_pipeline {

const char* VisualPipelineModeName(VisualPipelineMode mode) {
  switch (mode) {
    case VisualPipelineMode::kUsePreparedVisualMap:
      return "use_prepared_visual_map";
    case VisualPipelineMode::kBuildCpp:
      return "build_cpp";
    case VisualPipelineMode::kHybrid:
      return "hybrid";
    case VisualPipelineMode::kCompare:
      return "compare";
  }

  return "use_prepared_visual_map";
}

std::optional<VisualPipelineMode> ParseVisualPipelineMode(
    std::string_view value) {
  if (value == "use_prepared_visual_map" || value == "prepared" ||
      value == "visual_map") {
    return VisualPipelineMode::kUsePreparedVisualMap;
  }
  if (value == "build_cpp" || value == "cpp") {
    return VisualPipelineMode::kBuildCpp;
  }
  if (value == "hybrid") {
    return VisualPipelineMode::kHybrid;
  }
  if (value == "compare") {
    return VisualPipelineMode::kCompare;
  }

  return std::nullopt;
}

}  // namespace sar::visual_pipeline
