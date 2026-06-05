#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_TERRAIN_REGIONS_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_TERRAIN_REGIONS_H_

#include <cstdint>
#include <string>
#include <vector>

#include "level/level_data.h"
#include "level/terrain_type.h"
#include "visual_pipeline/semantic_masks.h"

namespace sar::visual_pipeline {

struct TerrainRegion {
  int id = 0;
  TerrainType type = TerrainType::kUnknown;
  int area = 0;
  int min_x = 0;
  int min_y = 0;
  int max_x = 0;
  int max_y = 0;
  int inner_tile_count = 0;
  int border_tile_count = 0;
  std::vector<int> tile_indices;
  std::vector<int> border_tile_indices;

  /**
   * @brief Returns a readable dump of the terrain region.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

struct TerrainRegionSummary {
  int total_regions = 0;
  int open_ground_regions = 0;
  int forest_regions = 0;
  int road_regions = 0;
  int swamp_regions = 0;
  int water_regions = 0;
  int ruins_regions = 0;
  int wall_regions = 0;
  int unknown_regions = 0;
  int tiny_regions = 0;
  int largest_region_area = 0;
  int largest_forest_area = 0;
  int largest_open_ground_area = 0;

  /**
   * @brief Returns a readable summary of terrain region counters.
   *
   * @return String representation for logs and debug overlays.
   */
  std::string Dump() const;
};

struct TerrainRegions {
  LevelSize size;
  std::vector<TerrainRegion> regions;
  TerrainRegionSummary summary;

  /**
   * @brief Returns true when region data matches the level dimensions.
   *
   * @return Validation flag.
   */
  bool IsValid() const;

  /**
   * @brief Returns a readable dump of all terrain region counters.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

/**
 * @brief Builds connected terrain regions from semantic terrain masks.
 *
 * The builder uses 4-neighbor connected components and stores both full tile
 * membership and border tile membership for later visual smoothing passes.
 *
 * @param masks Semantic masks produced from the raw level.
 * @param error Error text populated when building fails.
 * @return Built terrain regions. Returned data is invalid on failure.
 */
TerrainRegions BuildTerrainRegions(const SemanticMasks& masks,
                                    std::string* error);

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_TERRAIN_REGIONS_H_
