#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_FOREST_VISUAL_PLAN_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_FOREST_VISUAL_PLAN_H_

#include <cstdint>
#include <string>
#include <vector>

#include "level/level_data.h"
#include "visual_pipeline/semantic_masks.h"
#include "visual_pipeline/terrain_regions.h"

namespace sar::visual_pipeline {

enum class ForestDepthBand : std::uint8_t {
  kNone = 0,
  kEdge = 1,
  kMid = 2,
  kDeep = 3,
};

enum class ClearingRole : std::uint8_t {
  kNone = 0,
  kMainClearing = 1,
  kSideClearing = 2,
  kConnectorCorridor = 3,
  kMicroClearing = 4,
  kSceneSpace = 5,
};

/**
 * @brief Returns the stable name for a forest depth band.
 *
 * @param band Forest depth band value.
 * @return Stable lowercase band name.
 */
const char* ForestDepthBandName(ForestDepthBand band);

/**
 * @brief Returns the stable name for a clearing role.
 *
 * @param role Clearing role value.
 * @return Stable lowercase role name.
 */
const char* ClearingRoleName(ClearingRole role);

struct ForestVisualSummary {
  int forest_tiles = 0;
  int forest_edge_tiles = 0;
  int forest_mid_tiles = 0;
  int forest_deep_tiles = 0;
  int route_influenced_tiles = 0;
  int main_clearing_tiles = 0;
  int side_clearing_tiles = 0;
  int connector_corridor_tiles = 0;
  int micro_clearing_tiles = 0;
  int scene_space_tiles = 0;

  /**
   * @brief Returns a readable dump of forest visual counters.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

struct ForestVisualPlan {
  LevelSize size;
  std::vector<std::uint8_t> forest_depth;
  std::vector<std::uint8_t> forest_edges;
  std::vector<std::uint8_t> clearing_roles;
  std::vector<std::uint8_t> route_influence;
  ForestVisualSummary summary;

  /**
   * @brief Returns true when the plan matches the level dimensions.
   *
   * @return Validation flag.
   */
  bool IsValid() const;

  /**
   * @brief Returns a readable dump of the forest visual plan.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

/**
 * @brief Builds forest depth bands and clearing roles from level semantics.
 *
 * This pass does not change gameplay data. It derives visual-only forest
 * depth, edge and clearing classification data from masks, regions, routes,
 * places and runtime object footprints.
 *
 * @param level Loaded level data with semantic layers.
 * @param masks Semantic masks produced from the raw level.
 * @param regions Connected terrain regions produced from semantic masks.
 * @param error Error text populated when building fails.
 * @return Built forest visual plan. Returned data is invalid on failure.
 */
ForestVisualPlan BuildForestVisualPlan(const LevelData& level,
                                       const SemanticMasks& masks,
                                       const TerrainRegions& regions,
                                       std::string* error);

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_FOREST_VISUAL_PLAN_H_
