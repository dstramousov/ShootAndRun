#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_PIPELINE_PROGRESS_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_PIPELINE_PROGRESS_H_

#include <string>

namespace sar::visual_pipeline {

struct PipelineProgress {
  int completed_steps = 0;
  int total_steps = 0;
  std::string current_step_name;
  bool running = false;
  bool finished = false;
  bool failed = false;
  std::string error;

  /**
   * @brief Returns normalized progress in range 0..1.
   *
   * @return Progress value suitable for progress bars.
   */
  float Normalized() const;

  /**
   * @brief Returns a readable dump of the pipeline progress.
   *
   * @return String representation for logs and diagnostics.
   */
  std::string Dump() const;
};

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_PIPELINE_PROGRESS_H_
