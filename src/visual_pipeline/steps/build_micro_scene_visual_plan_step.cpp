/**
 * @file src/visual_pipeline/steps/build_micro_scene_visual_plan_step.cpp
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains
 * implementation for build_micro_scene_visual_plan_step.cpp.
 */

#include "visual_pipeline/steps/build_micro_scene_visual_plan_step.h"

#include "visual_pipeline/micro_scene_visual_plan.h"

namespace sar::visual_pipeline {

/**
 * @brief Runs build micro scene visual plan step.
 */
bool RunBuildMicroSceneVisualPlanStep(const LevelData& level,
                                      PreparedLevel* prepared_level,
                                      std::string* error) {
  if (prepared_level == nullptr) {
    if (error != nullptr) {
      *error = "prepared level is null";
    }
    return false;
  }
  if (!prepared_level->semantic_masks.IsValid() ||
      !prepared_level->object_visual_plan.IsValid()) {
    if (error != nullptr) {
      *error = "micro-scene step requires semantic and object visual plans";
    }
    return false;
  }

  prepared_level->micro_scene_visual_plan = BuildMicroSceneVisualPlan(
      level, prepared_level->semantic_masks, prepared_level->forest_visual_plan,
      prepared_level->road_visual_plan, prepared_level->ruin_visual_plan,
      prepared_level->water_visual_plan, prepared_level->object_visual_plan,
      error);

  if (!prepared_level->micro_scene_visual_plan.IsValid()) {
    if (error != nullptr && error->empty()) {
      *error = "micro-scene visual plan is invalid";
    }
    return false;
  }
  return true;
}

}  // namespace sar::visual_pipeline
