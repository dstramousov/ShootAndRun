#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_STEPS_BUILD_TERRAIN_REGIONS_STEP_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_STEPS_BUILD_TERRAIN_REGIONS_STEP_H_

/**
 * @file src/visual_pipeline/steps/build_terrain_regions_step.h
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains public
 * declarations for build_terrain_regions_step.h.
 */

#include <string>

#include "visual_pipeline/prepared_level.h"

namespace sar::visual_pipeline {

/**
 * @brief Builds connected terrain regions and stores them in the prepared level.
 *
 * @param prepared_level Prepared visual level to read masks from and update.
 * @param error Error text populated when the step fails.
 * @return True when the step succeeds.
 */
bool RunBuildTerrainRegionsStep(PreparedLevel* prepared_level,
                                std::string* error);

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_STEPS_BUILD_TERRAIN_REGIONS_STEP_H_
