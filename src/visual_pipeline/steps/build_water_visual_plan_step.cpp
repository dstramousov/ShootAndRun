#include "visual_pipeline/steps/build_water_visual_plan_step.h"

#include "visual_pipeline/water_visual_plan.h"

namespace sar::visual_pipeline {

bool RunBuildWaterVisualPlanStep(const LevelData& level,
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
      *error = "semantic masks must be built before water visual plan";
    }
    return false;
  }

  prepared_level->water_visual_plan = BuildWaterVisualPlan(
      level, prepared_level->semantic_masks, error);
  return prepared_level->water_visual_plan.IsValid();
}

}  // namespace sar::visual_pipeline
