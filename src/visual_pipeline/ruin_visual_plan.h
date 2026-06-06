#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_RUIN_VISUAL_PLAN_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_RUIN_VISUAL_PLAN_H_

#include <cstdint>
#include <string>
#include <vector>

#include "level/level_data.h"
#include "visual_pipeline/semantic_masks.h"

namespace sar::visual_pipeline {

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

struct RuinVisualSummary {
  int site_count = 0;
  int source_ruin_tiles = 0;
  int source_wall_tiles = 0;
  int cracked_floor_tiles = 0;
  int overgrown_floor_tiles = 0;
  int wall_intact_tiles = 0;
  int wall_broken_tiles = 0;
  int wall_corner_tiles = 0;
  int wall_endcap_tiles = 0;
  int rubble_tiles = 0;
  int entrance_tiles = 0;
  int visual_tiles = 0;

  /**
   * @brief Returns a readable dump of ruin visual counters.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

struct RuinVisualPlan {
  LevelSize size;
  std::vector<std::uint8_t> tiles;
  std::vector<std::uint16_t> site_ids;
  RuinVisualSummary summary;

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
