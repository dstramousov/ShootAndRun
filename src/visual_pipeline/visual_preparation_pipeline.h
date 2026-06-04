#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_VISUAL_PREPARATION_PIPELINE_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_VISUAL_PREPARATION_PIPELINE_H_

#include <string>
#include <vector>

#include "level/level_data.h"
#include "visual_pipeline/pipeline_progress.h"
#include "visual_pipeline/prepared_level.h"

namespace sar::visual_pipeline {

struct PipelineStepInfo {
  std::string name;
};

class VisualPreparationPipeline {
 public:
  /**
   * @brief Resets the pipeline and prepares it for a new level.
   *
   * @param level Raw loaded level data.
   */
  void Start(const LevelData& level);

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
  void RunCurrentStep(const LevelData& level, const PipelineStepInfo& step);
  void Fail(std::string error);

  std::vector<PipelineStepInfo> steps_;
  PipelineProgress progress_;
  PreparedLevel prepared_level_;
};

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_VISUAL_PREPARATION_PIPELINE_H_
