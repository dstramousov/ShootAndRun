#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_RUIN_VISUAL_PLAN_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_RUIN_VISUAL_PLAN_H_

/**
 * @file src/visual_pipeline/ruin_visual_plan.h
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains public
 * declarations for ruin_visual_plan.h.
 */

#include <cstdint>
#include <string>
#include <vector>

#include "level/level_data.h"
#include "visual_pipeline/semantic_masks.h"

namespace sar::visual_pipeline {

/**
 * @brief Visual role assigned to a ruin tile.
 */
enum class RuinVisualTile : std::uint8_t {
  kNone = 0,
  kCrackedFloor = 1,
  kOvergrownFloor = 2,
  kWallIntact = 3,
  kWallBroken = 4,
  kWallCorner = 5,
  kWallEndcap = 6,
  kRubble = 7,
  kEntrance = 8,
};

/**
 * @brief Counters produced by the ruin visual planning pass.
 */
struct RuinVisualSummary {
  int site_count = 0;  ///< Count of site count entries or events.
  int source_ruin_tiles = 0;  ///< Source ruin tiles value carried by this data structure.
  int source_wall_tiles = 0;  ///< Source wall tiles value carried by this data structure.
  int cracked_floor_tiles = 0;  ///< Cracked floor tiles value carried by this data structure.
  int overgrown_floor_tiles = 0;  ///< Overgrown floor tiles value carried by this data structure.
  int wall_intact_tiles = 0;  ///< Wall intact tiles value carried by this data structure.
  int wall_broken_tiles = 0;  ///< Wall broken tiles value carried by this data structure.
  int wall_corner_tiles = 0;  ///< Wall corner tiles value carried by this data structure.
  int wall_endcap_tiles = 0;  ///< Wall endcap tiles value carried by this data structure.
  int rubble_tiles = 0;  ///< Rubble tiles value carried by this data structure.
  int entrance_tiles = 0;  ///< Entrance tiles value carried by this data structure.
  int visual_tiles = 0;  ///< Visual tiles value carried by this data structure.

  /**
   * @brief Returns a readable dump of ruin visual counters.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

/**
 * @brief Planned ruin floor, wall and debris masks.
 */
struct RuinVisualPlan {
  LevelSize size;  ///< Size value carried by this data structure.
  std::vector<std::uint8_t> tiles;  ///< Tiles value carried by this data structure.
  std::vector<std::uint16_t> site_ids;  ///< Site ids value carried by this data structure.
  RuinVisualSummary summary;  ///< Summary value carried by this data structure.

  /**
   * @brief Returns true when the plan matches the level dimensions.
   *
   * @return Validation flag.
   */
  bool IsValid() const;

  /**
   * @brief Returns a readable dump of the ruin visual plan.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

/**
 * @brief Returns the stable name for a ruin visual tile role.
 *
 * @param tile Ruin visual tile value.
 * @return Stable lowercase tile role name.
 */
const char* RuinVisualTileName(RuinVisualTile tile);

/**
 * @brief Builds visual-only ruin scene composition data.
 *
 * The pass keeps gameplay data unchanged. It groups wall and ruin-floor
 * cells into sites, classifies wall pieces, finds local floor/rubble zones
 * and prepares a less technical preview representation.
 *
 * @param level Loaded level data.
 * @param masks Semantic masks produced from the raw level.
 * @param error Error text populated when building fails.
 * @return Built ruin visual plan. Returned data is invalid on failure.
 */
RuinVisualPlan BuildRuinVisualPlan(const LevelData& level,
                                   const SemanticMasks& masks,
                                   std::string* error);

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_RUIN_VISUAL_PLAN_H_
