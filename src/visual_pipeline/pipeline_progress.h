#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_PIPELINE_PROGRESS_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_PIPELINE_PROGRESS_H_

/**
 * @file src/visual_pipeline/pipeline_progress.h
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains public
 * declarations for pipeline_progress.h.
 */

#include <string>

namespace sar::visual_pipeline {

/**
 * @brief Stores pipeline progress data shared between runtime systems.
 */
struct PipelineProgress {
  int completed_steps = 0;  ///< Completed steps value carried by this data structure.
  int total_steps = 0;  ///< Total steps value carried by this data structure.
  std::string current_step_name;  ///< Current step name value carried by this data structure.
  bool running = false;  ///< Running value carried by this data structure.
  bool finished = false;  ///< Finished value carried by this data structure.
  bool failed = false;  ///< Failed value carried by this data structure.
  std::string error;  ///< Human-readable error message when loading or validation fails.

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
