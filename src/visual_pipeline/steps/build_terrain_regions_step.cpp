#include "visual_pipeline/steps/build_terrain_regions_step.h"

#include <string>
#include <utility>

#include "visual_pipeline/terrain_regions.h"

namespace sar::visual_pipeline {

bool RunBuildTerrainRegionsStep(PreparedLevel* prepared_level,
                                std::string* error) {
  if (prepared_level == nullptr) {
    if (error != nullptr) {
      *error = "prepared level is null";
    }
    return false;
  }

  if (!prepared_level->semantic_masks.IsValid()) {
    if (error != nullptr) {
      *error = "semantic masks must be built before terrain regions";
    }
    return false;
  }

  std::string build_error;
  TerrainRegions regions = BuildTerrainRegions(prepared_level->semantic_masks,
                                               &build_error);
  if (!regions.IsValid()) {
    if (error != nullptr) {
      *error = build_error.empty() ? "terrain regions are invalid" :
                                    build_error;
    }
    return false;
  }

  prepared_level->terrain_regions = std::move(regions);
  prepared_level->terrain_region_count =
      static_cast<int>(prepared_level->terrain_regions.regions.size());
  return true;
}

}  // namespace sar::visual_pipeline
