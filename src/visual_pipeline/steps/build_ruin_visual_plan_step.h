#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_STEPS_BUILD_RUIN_VISUAL_PLAN_STEP_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_STEPS_BUILD_RUIN_VISUAL_PLAN_STEP_H_

/**
 * @file src/visual_pipeline/steps/build_ruin_visual_plan_step.h
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains public
 * declarations for build_ruin_visual_plan_step.h.
 */

#include <string>

#include "level/level_data.h"
#include "visual_pipeline/prepared_level.h"

namespace sar::visual_pipeline {

/**
 * @brief Runs the ruin visual plan build step.
 *
 * @param level Loaded level data with ruin and wall semantics.
 * @param prepared_level Prepared level state updated with ruin visual data.
 * @param error Error text populated on failure.
 * @return True when the step succeeds.
 */
bool RunBuildRuinVisualPlanStep(const LevelData& level,
                                PreparedLevel* prepared_level,
                                std::string* error);

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_STEPS_BUILD_RUIN_VISUAL_PLAN_STEP_H_
