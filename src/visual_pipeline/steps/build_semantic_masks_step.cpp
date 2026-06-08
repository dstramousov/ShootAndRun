/**
 * @file src/visual_pipeline/steps/build_semantic_masks_step.cpp
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains
 * implementation for build_semantic_masks_step.cpp.
 */

#include "visual_pipeline/steps/build_semantic_masks_step.h"

#include <string>
#include <utility>

#include "visual_pipeline/semantic_masks.h"

namespace sar::visual_pipeline {

/**
 * @brief Runs build semantic masks step.
 */
bool RunBuildSemanticMasksStep(const LevelData& level,
                               PreparedLevel* prepared_level,
                               std::string* error) {
  if (prepared_level == nullptr) {
    if (error != nullptr) {
      *error = "prepared level is null";
    }
    return false;
  }

  std::string build_error;
  SemanticMasks masks = BuildSemanticMasks(level, &build_error);
  if (!masks.IsValid()) {
    if (error != nullptr) {
      *error = build_error.empty() ? "semantic masks are invalid" : build_error;
    }
    return false;
  }

  prepared_level->semantic_masks = std::move(masks);
  prepared_level->semantic_mask_count =
      prepared_level->semantic_masks.BoolMaskCount();
  return true;
}

}  // namespace sar::visual_pipeline
