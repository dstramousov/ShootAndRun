#include "visual_pipeline/steps/build_forest_visual_plan_step.h"

#include "visual_pipeline/forest_visual_plan.h"

namespace sar::visual_pipeline {

bool RunBuildForestVisualPlanStep(const LevelData& level,
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
      *error = "semantic masks must be built before forest visual plan";
    }
    return false;
  }
  if (!prepared_level->terrain_regions.IsValid()) {
    if (error != nullptr) {
      *error = "terrain regions must be built before forest visual plan";
    }
    return false;
  }

  prepared_level->forest_visual_plan = BuildForestVisualPlan(
      level, prepared_level->semantic_masks, prepared_level->terrain_regions,
      error);
  return prepared_level->forest_visual_plan.IsValid();
}

}  // namespace sar::visual_pipeline
