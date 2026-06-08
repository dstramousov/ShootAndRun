#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_STEPS_BUILD_OBJECT_VISUAL_PLAN_STEP_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_STEPS_BUILD_OBJECT_VISUAL_PLAN_STEP_H_

/**
 * @file src/visual_pipeline/steps/build_object_visual_plan_step.h
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains public
 * declarations for build_object_visual_plan_step.h.
 */

#include <string>

#include "level/level_data.h"
#include "visual_pipeline/prepared_level.h"

namespace sar::visual_pipeline {

/**
 * @brief Builds and stores a typed object visual mapping for a prepared level.
 *
 * @param level Loaded level data with runtime objects.
 * @param prepared_level Prepared level to update.
 * @param error Error text populated when the step fails.
 * @return True when the step succeeds.
 */
bool RunBuildObjectVisualPlanStep(const LevelData& level,
                                  PreparedLevel* prepared_level,
                                  std::string* error);

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_STEPS_BUILD_OBJECT_VISUAL_PLAN_STEP_H_
