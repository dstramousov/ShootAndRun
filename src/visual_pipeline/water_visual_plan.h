#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_WATER_VISUAL_PLAN_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_WATER_VISUAL_PLAN_H_

/**
 * @file src/visual_pipeline/water_visual_plan.h
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains public
 * declarations for water_visual_plan.h.
 */

#include <cstdint>
#include <string>
#include <vector>

#include "level/level_data.h"
#include "visual_pipeline/semantic_masks.h"

namespace sar::visual_pipeline {

/**
 * @brief Visual role assigned to a water or swamp tile.
 */
enum class WaterVisualTile : std::uint8_t {
  kNone = 0,
  kWetGrass = 1,
  kMudRing = 2,
  kReedZone = 3,
  kCrossing = 4,
  kWaterEdge = 5,
  kWaterCore = 6,
};

/**
 * @brief Counters produced by the water visual planning pass.
 */
struct WaterVisualSummary {
  int source_water_tiles = 0;  ///< Source water tiles value carried by this data structure.
  int source_swamp_tiles = 0;  ///< Source swamp tiles value carried by this data structure.
  int water_like_tiles = 0;  ///< Water like tiles value carried by this data structure.
  int water_region_count = 0;  ///< Count of water region count entries or events.
  int water_core_tiles = 0;  ///< Water core tiles value carried by this data structure.
  int water_edge_tiles = 0;  ///< Water edge tiles value carried by this data structure.
  int mud_ring_tiles = 0;  ///< Mud ring tiles value carried by this data structure.
  int wet_grass_tiles = 0;  ///< Wet grass tiles value carried by this data structure.
  int reed_zone_tiles = 0;  ///< Reed zone tiles value carried by this data structure.
  int crossing_tiles = 0;  ///< Crossing tiles value carried by this data structure.
  int visual_tiles = 0;  ///< Visual tiles value carried by this data structure.

  /**
   * @brief Returns a readable dump of water visual counters.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

/**
 * @brief Planned water core, shore and reed-zone masks.
 */
struct WaterVisualPlan {
  LevelSize size;  ///< Size value carried by this data structure.
  std::vector<std::uint8_t> tiles;  ///< Tiles value carried by this data structure.
  std::vector<std::uint16_t> region_ids;  ///< Region ids value carried by this data structure.
  WaterVisualSummary summary;  ///< Summary value carried by this data structure.

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
