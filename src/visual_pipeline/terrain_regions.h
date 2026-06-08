#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_TERRAIN_REGIONS_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_TERRAIN_REGIONS_H_

/**
 * @file src/visual_pipeline/terrain_regions.h
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains public
 * declarations for terrain_regions.h.
 */

#include <cstdint>
#include <string>
#include <vector>

#include "level/level_data.h"
#include "level/terrain_type.h"
#include "visual_pipeline/semantic_masks.h"

namespace sar::visual_pipeline {

/**
 * @brief One connected terrain component extracted from semantic masks.
 */
struct TerrainRegion {
  int id = 0;  ///< Stable identifier loaded from source data or configuration.
  TerrainType type = TerrainType::kUnknown;  ///< Semantic type loaded from source data or configuration.
  int area = 0;  ///< Area value carried by this data structure.
  int min_x = 0;  ///< Tile, screen, or world coordinate for min x.
  int min_y = 0;  ///< Tile, screen, or world coordinate for min y.
  int max_x = 0;  ///< Tile, screen, or world coordinate for max x.
  int max_y = 0;  ///< Tile, screen, or world coordinate for max y.
  int inner_tile_count = 0;  ///< Count of inner tile count entries or events.
  int border_tile_count = 0;  ///< Count of border tile count entries or events.
  std::vector<int> tile_indices;  ///< Tile indices value carried by this data structure.
  std::vector<int> border_tile_indices;  ///< Border tile indices value carried by this data structure.

  /**
   * @brief Returns a readable dump of the terrain region.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

/**
 * @brief Counters describing connected terrain regions.
 */
struct TerrainRegionSummary {
  int total_regions = 0;  ///< Total regions value carried by this data structure.
  int open_ground_regions = 0;  ///< Open ground regions value carried by this data structure.
  int forest_regions = 0;  ///< Forest regions value carried by this data structure.
  int road_regions = 0;  ///< Road regions value carried by this data structure.
  int swamp_regions = 0;  ///< Swamp regions value carried by this data structure.
  int water_regions = 0;  ///< Water regions value carried by this data structure.
  int ruins_regions = 0;  ///< Ruins regions value carried by this data structure.
  int wall_regions = 0;  ///< Wall regions value carried by this data structure.
  int unknown_regions = 0;  ///< Unknown regions value carried by this data structure.
  int tiny_regions = 0;  ///< Tiny regions value carried by this data structure.
  int largest_region_area = 0;  ///< Largest region area value carried by this data structure.
  int largest_forest_area = 0;  ///< Largest forest area value carried by this data structure.
  int largest_open_ground_area = 0;  ///< Largest open ground area value carried by this data structure.

  /**
   * @brief Returns a readable summary of terrain region counters.
   *
   * @return String representation for logs.
   */
  std::string Dump() const;
};

/**
 * @brief Collection of connected terrain regions for a level.
 */
struct TerrainRegions {
  LevelSize size;  ///< Size value carried by this data structure.
  std::vector<TerrainRegion> regions;  ///< Regions value carried by this data structure.
  TerrainRegionSummary summary;  ///< Summary value carried by this data structure.

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
