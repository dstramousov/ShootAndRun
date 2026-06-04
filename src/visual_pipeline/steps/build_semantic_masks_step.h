#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_STEPS_BUILD_SEMANTIC_MASKS_STEP_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_STEPS_BUILD_SEMANTIC_MASKS_STEP_H_

#include <string>

#include "level/level_data.h"
#include "visual_pipeline/prepared_level.h"

namespace sar::visual_pipeline {

/**
 * @brief Builds semantic masks and stores them in the prepared level.
 *
 * @param level Raw loaded level data.
 * @param prepared_level Prepared visual level to update.
 * @param error Error text populated when the step fails.
 * @return True when the step succeeds.
 */
bool RunBuildSemanticMasksStep(const LevelData& level,
                               PreparedLevel* prepared_level,
                               std::string* error);

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_STEPS_BUILD_SEMANTIC_MASKS_STEP_H_
