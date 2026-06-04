#include "visual_pipeline/pipeline_progress.h"

#include <algorithm>
#include <string>

namespace sar::visual_pipeline {

float PipelineProgress::Normalized() const {
  if (total_steps <= 0) {
    return finished ? 1.0F : 0.0F;
  }

  const float value = static_cast<float>(completed_steps) /
                      static_cast<float>(total_steps);
  return std::clamp(value, 0.0F, 1.0F);
}

std::string PipelineProgress::Dump() const {
  return "PipelineProgress { completed: " +
         std::to_string(completed_steps) + "/" +
         std::to_string(total_steps) + ", current_step: \"" +
         current_step_name + "\", running: " +
         std::string(running ? "true" : "false") + ", finished: " +
         std::string(finished ? "true" : "false") + ", failed: " +
         std::string(failed ? "true" : "false") + " }";
}

}  // namespace sar::visual_pipeline
