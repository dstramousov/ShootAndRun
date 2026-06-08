#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_ROAD_VISUAL_PLAN_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_ROAD_VISUAL_PLAN_H_

/**
 * @file src/visual_pipeline/road_visual_plan.h
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains public
 * declarations for road_visual_plan.h.
 */

#include <cstdint>
#include <string>
#include <vector>

#include "level/level_data.h"
#include "visual_pipeline/semantic_masks.h"

namespace sar::visual_pipeline {

/**
 * @brief Visual band assigned around road and route influence tiles.
 */
enum class RoadVisualBand : std::uint8_t {
  kNone = 0,
  kTrampledGrass = 1,
  kRoadSide = 2,
  kRoadCore = 3,
  kMudPatch = 4,
  kRuinApproach = 5,
};

/**
 * @brief Counters produced by the road visual planning pass.
 */
struct RoadVisualSummary {
  int route_count = 0;  ///< Count of route count entries or events.
  int main_route_count = 0;  ///< Count of main route count entries or events.
  int side_route_count = 0;  ///< Count of side route count entries or events.
  int hidden_route_count = 0;  ///< Count of hidden route count entries or events.
  int terrain_road_tiles = 0;  ///< Terrain road tiles value carried by this data structure.
  int road_dressing_tiles = 0;  ///< Road dressing tiles value carried by this data structure.
  int road_core_tiles = 0;  ///< Road core tiles value carried by this data structure.
  int road_side_tiles = 0;  ///< Road side tiles value carried by this data structure.
  int trampled_grass_tiles = 0;  ///< Trampled grass tiles value carried by this data structure.
  int mud_patch_tiles = 0;  ///< Mud patch tiles value carried by this data structure.
  int ruin_approach_tiles = 0;  ///< Ruin approach tiles value carried by this data structure.
  int route_influenced_tiles = 0;  ///< Route influenced tiles value carried by this data structure.
  bool routes_used_for_visual_roads = false;  ///< Routes used for visual roads value carried by this data structure.

  /**
   * @brief Returns a readable dump of road visual counters.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

/**
 * @brief Planned road influence and visual band masks.
 */
struct RoadVisualPlan {
  LevelSize size;  ///< Size value carried by this data structure.
  std::vector<std::uint8_t> road_bands;  ///< Road bands value carried by this data structure.
  std::vector<std::uint8_t> route_influence;  ///< Route influence value carried by this data structure.
  RoadVisualSummary summary;  ///< Summary value carried by this data structure.

  /**
   * @brief Returns true when the plan matches the level dimensions.
   *
   * @return Validation flag.
   */
  bool IsValid() const;

  /**
   * @brief Returns a readable dump of the road visual plan.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

/**
 * @brief Returns the stable name for a road visual band.
 *
 * @param band Road visual band value.
 * @return Stable lowercase band name.
 */
const char* RoadVisualBandName(RoadVisualBand band);

/**
 * @brief Builds visual road dressing from existing road terrain.
 *
 * This pass is visual-only. It derives road core, soft edges, trampled
 * grass, mud patches and ruin approaches from terrain road tiles. Routes
 * are counted for diagnostics only and are not painted as roads.
 *
 * @param level Loaded level data with terrain, routes and semantic layers.
 * @param masks Semantic masks produced from the raw level.
 * @param error Error text populated when building fails.
 * @return Built road visual plan. Returned data is invalid on failure.
 */
RoadVisualPlan BuildRoadVisualPlan(const LevelData& level,
                                   const SemanticMasks& masks,
                                   std::string* error);

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_ROAD_VISUAL_PLAN_H_
