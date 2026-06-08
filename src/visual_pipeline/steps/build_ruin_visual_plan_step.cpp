/**
 * @file src/visual_pipeline/steps/build_ruin_visual_plan_step.cpp
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains
 * implementation for build_ruin_visual_plan_step.cpp.
 */

#include "visual_pipeline/steps/build_ruin_visual_plan_step.h"

#include "visual_pipeline/ruin_visual_plan.h"

namespace sar::visual_pipeline {

/**
 * @brief Runs build ruin visual plan step.
 */
bool RunBuildRuinVisualPlanStep(const LevelData& level,
                                PreparedLevel* prepared_level,
                                std::string* error) {
  if (prepared_level == nullptr) {
    if (error != nullptr) {
      *error = "prepared level is null";
    }
    return false;
  }
  if (!prepared_level->semantic_masks.IsValid()) {
    if (error != nullptr) {
      *error = "semantic masks must be built before ruin visual plan";
    }
    return false;
  }

  prepared_level->ruin_visual_plan = BuildRuinVisualPlan(
      level, prepared_level->semantic_masks, error);
  return prepared_level->ruin_visual_plan.IsValid();
}

}  // namespace sar::visual_pipeline
