#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_VISUAL_PREPARATION_PIPELINE_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_VISUAL_PREPARATION_PIPELINE_H_

/**
 * @file src/visual_pipeline/visual_preparation_pipeline.h
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains public
 * declarations for visual_preparation_pipeline.h.
 */

#include <filesystem>
#include <string>
#include <vector>

#include "level/level_data.h"
#include "visual_pipeline/pipeline_progress.h"
#include "visual_pipeline/prepared_level.h"
#include "visual_pipeline/visual_pipeline_config.h"

namespace sar::visual_pipeline {

/**
 * @brief Static metadata for one visual preparation pipeline step.
 */
struct PipelineStepInfo {
  std::string name;  ///< Human-readable name or configuration key.
};

/**
 * @brief Input options used by the visual preparation pipeline.
 */
struct VisualPreparationOptions {
  std::filesystem::path map_package_path;  ///< Filesystem path used by this configuration or data object.
  VisualPipelineConfig visual_pipeline_config;  ///< Visual pipeline config value carried by this data structure.
};

/**
 * @brief Runtime report emitted after one pipeline step finishes.
 */
struct PipelineStepReport {
  int step_index = 0;  ///< Step index value carried by this data structure.
  int total_steps = 0;  ///< Total steps value carried by this data structure.
  std::string step_name;  ///< Step name value carried by this data structure.
  double duration_ms = 0.0;  ///< Time value for duration milliseconds.
  bool success = false;  ///< Success value carried by this data structure.
  std::vector<std::string> summaries;  ///< Summaries value carried by this data structure.
  std::vector<std::string> warnings;  ///< Recoverable validation warnings collected during loading.
};

/**
 * @brief Incremental visual-map preparation pipeline.
 */
class VisualPreparationPipeline {
 public:
  /**
   * @brief Resets the pipeline and prepares it for a new level.
   *
   * @param level Raw loaded level data.
   */
  void Start(const LevelData& level);

  /**
   * @brief Resets the pipeline and prepares it for a new level.
   *
   * @param level Raw loaded level data.
   * @param options Visual preparation options.
   */
  void Start(const LevelData& level, VisualPreparationOptions options);

  /**
   * @brief Advances the preparation pipeline by one step.
   *
   * @param level Raw loaded level data used as preparation input.
   */
  void AdvanceOneStep(const LevelData& level);

  /**
   * @brief Returns current preparation progress.
   *
   * @return Pipeline progress state.
   */
  const PipelineProgress& progress() const { return progress_; }

  /**
   * @brief Returns the last executed pipeline step report.
   *
   * @return Report for diagnostics and logging.
   */
  const PipelineStepReport& last_step_report() const {
    return last_step_report_;
  }

  /**
   * @brief Returns prepared level data built by the pipeline.
   *
   * @return Prepared visual level.
   */
  const PreparedLevel& prepared_level() const { return prepared_level_; }

  /**
   * @brief Returns true when the pipeline finished successfully.
   *
   * @return Success completion flag.
   */
  bool finished() const { return progress_.finished && !progress_.failed; }

  /**
   * @brief Returns true when the pipeline is currently running.
   *
   * @return Running flag.
   */
  bool running() const { return progress_.running; }

 private:
  /**
   * @brief Runs current step.
   *
   * @param level Loaded level data used as the source of truth.
   * @param step Input value required by the operation.
   * @param report Input value required by the operation.
   */
  void RunCurrentStep(const LevelData& level, const PipelineStepInfo& step,
                      PipelineStepReport* report);
  /**
   * @brief Marks the pipeline step result as failed and stores the diagnostic error message.
   *
   * @param error Input value required by the operation.
   */
  void Fail(std::string error);

  std::vector<PipelineStepInfo> steps_;
  PipelineProgress progress_;
  PipelineStepReport last_step_report_;
  PreparedLevel prepared_level_;
  VisualPreparationOptions options_;
};

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_VISUAL_PREPARATION_PIPELINE_H_
