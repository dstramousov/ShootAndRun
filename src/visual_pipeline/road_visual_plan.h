#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_ROAD_VISUAL_PLAN_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_ROAD_VISUAL_PLAN_H_

#include <cstdint>
#include <string>
#include <vector>

#include "level/level_data.h"
#include "visual_pipeline/semantic_masks.h"

namespace sar::visual_pipeline {

enum class RoadVisualBand : std::uint8_t {
  kNone = 0,
  kTrampledGrass = 1,
  kRoadSide = 2,
  kRoadCore = 3,
  kMudPatch = 4,
  kRuinApproach = 5,
};

struct RoadVisualSummary {
  int route_count = 0;
  int main_route_count = 0;
  int side_route_count = 0;
  int hidden_route_count = 0;
  int terrain_road_tiles = 0;
  int road_dressing_tiles = 0;
  int road_core_tiles = 0;
  int road_side_tiles = 0;
  int trampled_grass_tiles = 0;
  int mud_patch_tiles = 0;
  int ruin_approach_tiles = 0;
  int route_influenced_tiles = 0;
  bool routes_used_for_visual_roads = false;

  /**
   * @brief Returns a readable dump of road visual counters.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

struct RoadVisualPlan {
  LevelSize size;
  std::vector<std::uint8_t> road_bands;
  std::vector<std::uint8_t> route_influence;
  RoadVisualSummary summary;

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
