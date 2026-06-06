#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_WATER_VISUAL_PLAN_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_WATER_VISUAL_PLAN_H_

#include <cstdint>
#include <string>
#include <vector>

#include "level/level_data.h"
#include "visual_pipeline/semantic_masks.h"

namespace sar::visual_pipeline {

enum class WaterVisualTile : std::uint8_t {
  kNone = 0,
  kWetGrass = 1,
  kMudRing = 2,
  kReedZone = 3,
  kCrossing = 4,
  kWaterEdge = 5,
  kWaterCore = 6,
};

struct WaterVisualSummary {
  int source_water_tiles = 0;
  int source_swamp_tiles = 0;
  int water_like_tiles = 0;
  int water_region_count = 0;
  int water_core_tiles = 0;
  int water_edge_tiles = 0;
  int mud_ring_tiles = 0;
  int wet_grass_tiles = 0;
  int reed_zone_tiles = 0;
  int crossing_tiles = 0;
  int visual_tiles = 0;

  /**
   * @brief Returns a readable dump of water visual counters.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

struct WaterVisualPlan {
  LevelSize size;
  std::vector<std::uint8_t> tiles;
  std::vector<std::uint16_t> region_ids;
  WaterVisualSummary summary;

  /**
   * @brief Returns true when the plan matches the level dimensions.
   *
   * @return Validation flag.
   */
  bool IsValid() const;

  /**
   * @brief Returns a readable dump of the water visual plan.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

/**
 * @brief Returns the stable name for a water visual tile role.
 *
 * @param tile Water visual tile value.
 * @return Stable lowercase tile role name.
 */
const char* WaterVisualTileName(WaterVisualTile tile);

/**
 * @brief Builds visual-only water, mud, wet grass and reed zones.
 *
 * The pass keeps gameplay data unchanged. It uses existing water/swamp cells
 * as the source of truth and adds only visual intent around their edges.
 *
 * @param level Loaded level data.
 * @param masks Semantic masks produced from the raw level.
 * @param error Error text populated when building fails.
 * @return Built water visual plan. Returned data is invalid on failure.
 */
WaterVisualPlan BuildWaterVisualPlan(const LevelData& level,
                                     const SemanticMasks& masks,
                                     std::string* error);

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_WATER_VISUAL_PLAN_H_
