#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_STEPS_BUILD_MICRO_SCENE_VISUAL_PLAN_STEP_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_STEPS_BUILD_MICRO_SCENE_VISUAL_PLAN_STEP_H_

#include <string>

#include "level/level_data.h"
#include "visual_pipeline/prepared_level.h"

namespace sar::visual_pipeline {

/**
 * @brief Builds visual-only micro-scene dressing data for the prepared level.
 *
 * @param level Loaded level data.
 * @param prepared_level Prepared level data to update.
 * @param error Error text populated when the step fails.
 * @return True when the step completed successfully.
 */
bool RunBuildMicroSceneVisualPlanStep(const LevelData& level,
                                      PreparedLevel* prepared_level,
                                      std::string* error);

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_STEPS_BUILD_MICRO_SCENE_VISUAL_PLAN_STEP_H_
