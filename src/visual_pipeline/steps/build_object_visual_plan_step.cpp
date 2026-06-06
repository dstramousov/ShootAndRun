#include "visual_pipeline/steps/build_object_visual_plan_step.h"

#include "visual_pipeline/object_visual_plan.h"

namespace sar::visual_pipeline {

bool RunBuildObjectVisualPlanStep(const LevelData& level,
                                  PreparedLevel* prepared_level,
                                  std::string* error) {
  if (prepared_level == nullptr) {
    if (error != nullptr) {
      *error = "prepared level pointer is null";
    }
    return false;
  }

  ObjectVisualPlan plan = BuildObjectVisualPlan(level, error);
  if (!plan.IsValid()) {
    if (error != nullptr && error->empty()) {
      *error = "built object visual plan is invalid";
    }
    return false;
  }

  prepared_level->object_visual_plan = std::move(plan);
  return true;
}

}  // namespace sar::visual_pipeline
