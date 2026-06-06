#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_STEPS_BUILD_WATER_VISUAL_PLAN_STEP_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_STEPS_BUILD_WATER_VISUAL_PLAN_STEP_H_

#include <string>

#include "level/level_data.h"
#include "visual_pipeline/prepared_level.h"

namespace sar::visual_pipeline {

/**
 * @brief Runs the water visual plan build step.
 *
 * @param level Loaded level data with water and swamp semantics.
 * @param prepared_level Prepared level state updated with water visual data.
 * @param error Error text populated on failure.
 * @return True when the step succeeds.
 */
bool RunBuildWaterVisualPlanStep(const LevelData& level,
                                 PreparedLevel* prepared_level,
                                 std::string* error);

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_STEPS_BUILD_WATER_VISUAL_PLAN_STEP_H_
