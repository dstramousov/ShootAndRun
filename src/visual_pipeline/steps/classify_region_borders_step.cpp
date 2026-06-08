/**
 * @file src/visual_pipeline/steps/classify_region_borders_step.cpp
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains
 * implementation for classify_region_borders_step.cpp.
 */

#include "visual_pipeline/steps/classify_region_borders_step.h"

#include <string>
#include <utility>

#include "visual_pipeline/region_borders.h"

namespace sar::visual_pipeline {

/**
 * @brief Runs classify region borders step.
 */
bool RunClassifyRegionBordersStep(PreparedLevel* prepared_level,
                                  std::string* error) {
  if (prepared_level == nullptr) {
    if (error != nullptr) {
      *error = "prepared level is null";
    }
    return false;
  }

  if (!prepared_level->semantic_masks.IsValid()) {
    if (error != nullptr) {
      *error = "semantic masks must be built before region borders";
    }
    return false;
  }

  if (!prepared_level->terrain_regions.IsValid()) {
    if (error != nullptr) {
      *error = "terrain regions must be built before region borders";
    }
    return false;
  }

  std::string build_error;
  RegionBorders borders = ClassifyRegionBorders(
      prepared_level->semantic_masks, prepared_level->terrain_regions,
      &build_error);
  if (!borders.IsValid()) {
    if (error != nullptr) {
      *error = build_error.empty() ? "region borders are invalid" :
                                    build_error;
    }
    return false;
  }

  prepared_level->region_borders = std::move(borders);
  prepared_level->region_border_count =
      static_cast<int>(prepared_level->region_borders.regions.size());
  return true;
}

}  // namespace sar::visual_pipeline
