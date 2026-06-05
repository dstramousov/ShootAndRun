#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_STEPS_CLASSIFY_REGION_BORDERS_STEP_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_STEPS_CLASSIFY_REGION_BORDERS_STEP_H_

#include <string>

#include "visual_pipeline/prepared_level.h"

namespace sar::visual_pipeline {

/**
 * @brief Classifies terrain region border tiles for future smoothing passes.
 *
 * @param prepared_level Prepared level containing semantic masks and regions.
 * @param error Error text populated when the step fails.
 * @return True on success.
 */
bool RunClassifyRegionBordersStep(PreparedLevel* prepared_level,
                                  std::string* error);

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_STEPS_CLASSIFY_REGION_BORDERS_STEP_H_
