#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_REGION_BORDERS_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_REGION_BORDERS_H_

#include <string>
#include <vector>

#include "level/level_data.h"
#include "level/terrain_type.h"
#include "visual_pipeline/semantic_masks.h"
#include "visual_pipeline/terrain_regions.h"

namespace sar::visual_pipeline {

struct BorderTile {
  int tile_index = 0;
  int x = 0;
  int y = 0;
  int region_id = 0;
  TerrainType region_type = TerrainType::kUnknown;
  TerrainType north = TerrainType::kUnknown;
  TerrainType east = TerrainType::kUnknown;
  TerrainType south = TerrainType::kUnknown;
  TerrainType west = TerrainType::kUnknown;
  bool north_differs = false;
  bool east_differs = false;
  bool south_differs = false;
  bool west_differs = false;
  bool touches_map_edge = false;
  int differing_neighbor_count = 0;

  /**
   * @brief Returns true when the tile has an adjacent outside corner.
   *
   * @return Corner classification flag.
   */
  bool IsCorner() const;

  /**
   * @brief Returns a readable dump of the border tile.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

struct RegionBorderInfo {
  int region_id = 0;
  TerrainType type = TerrainType::kUnknown;
  int total_border_tiles = 0;
  int edge_tile_count = 0;
  int corner_tile_count = 0;
  int thin_tile_count = 0;
  int complex_tile_count = 0;
  int map_edge_tile_count = 0;
  int neighbor_open_ground = 0;
  int neighbor_forest = 0;
  int neighbor_road = 0;
  int neighbor_swamp = 0;
  int neighbor_water = 0;
  int neighbor_ruins = 0;
  int neighbor_wall = 0;
  int neighbor_unknown = 0;
  int neighbor_outside_map = 0;
  std::vector<BorderTile> tiles;

  /**
   * @brief Returns a readable dump of region border counters.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

struct RegionBorderSummary {
  int region_count = 0;
  int border_tile_count = 0;
  int edge_tile_count = 0;
  int corner_tile_count = 0;
  int thin_tile_count = 0;
  int complex_tile_count = 0;
  int map_edge_tile_count = 0;
  int neighbor_open_ground = 0;
  int neighbor_forest = 0;
  int neighbor_road = 0;
  int neighbor_swamp = 0;
  int neighbor_water = 0;
  int neighbor_ruins = 0;
  int neighbor_wall = 0;
  int neighbor_unknown = 0;
  int neighbor_outside_map = 0;

  /**
   * @brief Returns a readable summary of region border counters.
   *
   * @return String representation for logs and debug overlays.
   */
  std::string Dump() const;
};

struct RegionBorders {
  LevelSize size;
  std::vector<RegionBorderInfo> regions;
  RegionBorderSummary summary;

  /**
   * @brief Returns true when border data matches the terrain regions.
   *
   * @return Validation flag.
   */
  bool IsValid() const;

  /**
   * @brief Returns a readable dump of all border counters.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

/**
 * @brief Classifies terrain region borders for later smoothing passes.
 *
 * The classifier inspects every region border tile, records neighboring
 * terrain types, and marks edge/corner/thin/complex tile shapes. The output is
 * data-only and does not depend on raylib or rendering APIs.
 *
 * @param masks Semantic masks produced from the raw level.
 * @param terrain_regions Connected terrain regions built from the masks.
 * @param error Error text populated when classification fails.
 * @return Classified region borders. Returned data is invalid on failure.
 */
RegionBorders ClassifyRegionBorders(const SemanticMasks& masks,
                                     const TerrainRegions& terrain_regions,
                                     std::string* error);

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_REGION_BORDERS_H_
