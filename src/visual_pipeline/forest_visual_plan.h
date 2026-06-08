#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_FOREST_VISUAL_PLAN_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_FOREST_VISUAL_PLAN_H_

/**
 * @file src/visual_pipeline/forest_visual_plan.h
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains public
 * declarations for forest_visual_plan.h.
 */

#include <cstdint>
#include <string>
#include <vector>

#include "level/level_data.h"
#include "visual_pipeline/semantic_masks.h"
#include "visual_pipeline/terrain_regions.h"

namespace sar::visual_pipeline {

/**
 * @brief Relative depth band of a forest tile inside a forest region.
 */
enum class ForestDepthBand : std::uint8_t {
  kNone = 0,
  kEdge = 1,
  kMid = 2,
  kDeep = 3,
};

/**
 * @brief High-level role assigned to an open clearing.
 */
enum class ClearingRole : std::uint8_t {
  kNone = 0,
  kMainClearing = 1,
  kSideClearing = 2,
  kConnectorCorridor = 3,
  kMicroClearing = 4,
  kSceneSpace = 5,
};

/**
 * @brief Scene role used to dress an open clearing.
 */
enum class ClearingSceneRole : std::uint8_t {
  kNone = 0,
  kRuinsScene = 1,
  kRoadApproach = 2,
  kObjectScene = 3,
  kGenericScene = 4,
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

/**
 * @brief Returns the stable name for a clearing scene role.
 *
 * @param role Clearing scene role value.
 * @return Stable lowercase role name.
 */
const char* ClearingSceneRoleName(ClearingSceneRole role);

/**
 * @brief Counters produced by the forest visual planning pass.
 */
struct ForestVisualSummary {
  int forest_tiles = 0;  ///< Forest tiles value carried by this data structure.
  int forest_edge_tiles = 0;  ///< Forest edge tiles value carried by this data structure.
  int forest_mid_tiles = 0;  ///< Forest mid tiles value carried by this data structure.
  int forest_deep_tiles = 0;  ///< Forest deep tiles value carried by this data structure.
  int suppressed_tiny_forest_tiles = 0;  ///< Suppressed tiny forest tiles value carried by this data structure.
  int forest_mass_group_count = 0;  ///< Count of forest mass group count entries or events.
  int route_influenced_tiles = 0;  ///< Route influenced tiles value carried by this data structure.
  int main_clearing_tiles = 0;  ///< Main clearing tiles value carried by this data structure.
  int side_clearing_tiles = 0;  ///< Side clearing tiles value carried by this data structure.
  int connector_corridor_tiles = 0;  ///< Connector corridor tiles value carried by this data structure.
  int micro_clearing_tiles = 0;  ///< Micro clearing tiles value carried by this data structure.
  int scene_space_tiles = 0;  ///< Scene space tiles value carried by this data structure.
  int ruins_scene_tiles = 0;  ///< Ruins scene tiles value carried by this data structure.
  int road_approach_scene_tiles = 0;  ///< Road approach scene tiles value carried by this data structure.
  int object_scene_tiles = 0;  ///< Object scene tiles value carried by this data structure.
  int generic_scene_tiles = 0;  ///< Generic scene tiles value carried by this data structure.

  /**
   * @brief Returns a readable dump of forest visual counters.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

/**
 * @brief Planned forest depth, edge and clearing-role masks.
 */
struct ForestVisualPlan {
  LevelSize size;  ///< Size value carried by this data structure.
  std::vector<std::uint8_t> forest_depth;  ///< Forest depth value carried by this data structure.
  std::vector<std::uint8_t> forest_edges;  ///< Forest edges value carried by this data structure.
  std::vector<std::uint16_t> forest_mass_groups;  ///< Forest mass groups value carried by this data structure.
  std::vector<std::uint8_t> clearing_roles;  ///< Clearing roles value carried by this data structure.
  std::vector<std::uint8_t> clearing_scene_roles;  ///< Clearing scene roles value carried by this data structure.
  std::vector<std::uint8_t> route_influence;  ///< Route influence value carried by this data structure.
  ForestVisualSummary summary;  ///< Summary value carried by this data structure.

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
