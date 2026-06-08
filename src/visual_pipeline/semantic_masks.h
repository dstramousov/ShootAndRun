#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_SEMANTIC_MASKS_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_SEMANTIC_MASKS_H_

/**
 * @file src/visual_pipeline/semantic_masks.h
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains public
 * declarations for semantic_masks.h.
 */

#include <cstdint>
#include <string>
#include <vector>

#include "level/level_data.h"

namespace sar::visual_pipeline {

/**
 * @brief Counters describing semantic mask coverage for one level.
 */
struct SemanticMaskSummary {
  int total_tiles = 0;  ///< Total tiles value carried by this data structure.
  int open_ground_tiles = 0;  ///< Open ground tiles value carried by this data structure.
  int forest_tiles = 0;  ///< Forest tiles value carried by this data structure.
  int road_tiles = 0;  ///< Road tiles value carried by this data structure.
  int swamp_tiles = 0;  ///< Swamp tiles value carried by this data structure.
  int water_tiles = 0;  ///< Water tiles value carried by this data structure.
  int ruins_tiles = 0;  ///< Ruins tiles value carried by this data structure.
  int wall_tiles = 0;  ///< Wall tiles value carried by this data structure.
  int unknown_tiles = 0;  ///< Unknown tiles value carried by this data structure.
  int walkable_tiles = 0;  ///< Walkable tiles value carried by this data structure.
  int blocked_tiles = 0;  ///< Blocked tiles value carried by this data structure.
  int vision_blocked_tiles = 0;  ///< Vision blocked tiles value carried by this data structure.
  int projectile_blocked_tiles = 0;  ///< Projectile blocked tiles value carried by this data structure.
  int cover_tiles = 0;  ///< Cover tiles value carried by this data structure.
  int concealment_tiles = 0;  ///< Concealment tiles value carried by this data structure.
  int low_ground_tiles = 0;  ///< Low ground tiles value carried by this data structure.
  int elevated_tiles = 0;  ///< Elevated tiles value carried by this data structure.

  /**
   * @brief Returns a readable summary of semantic mask counters.
   *
   * @return String representation for logs.
   */
  std::string Dump() const;
};

/**
 * @brief Boolean and height masks derived from loaded runtime level data.
 */
struct SemanticMasks {
  LevelSize size;  ///< Size value carried by this data structure.
  std::vector<std::uint8_t> open_ground;  ///< Open ground value carried by this data structure.
  std::vector<std::uint8_t> forest;  ///< Forest value carried by this data structure.
  std::vector<std::uint8_t> road;  ///< Road value carried by this data structure.
  std::vector<std::uint8_t> swamp;  ///< Swamp value carried by this data structure.
  std::vector<std::uint8_t> water;  ///< Water value carried by this data structure.
  std::vector<std::uint8_t> ruins;  ///< Ruins value carried by this data structure.
  std::vector<std::uint8_t> wall;  ///< Wall value carried by this data structure.
  std::vector<std::uint8_t> unknown;  ///< Unknown value carried by this data structure.
  std::vector<std::uint8_t> walkable;  ///< true when movement is allowed by the movement grid.
  std::vector<std::uint8_t> blocked;  ///< Blocked value carried by this data structure.
  std::vector<std::uint8_t> vision_blocked;  ///< Vision blocked value carried by this data structure.
  std::vector<std::uint8_t> projectile_blocked;  ///< Projectile blocked value carried by this data structure.
  std::vector<std::uint8_t> cover;  ///< Cover strength encoded by the runtime grid.
  std::vector<std::uint8_t> concealment;  ///< Concealment strength encoded by the runtime grid.
  std::vector<std::int8_t> height;  ///< Signed elevation level for this tile or object.
  SemanticMaskSummary summary;  ///< Summary value carried by this data structure.

  /**
   * @brief Returns true when all semantic masks match the level dimensions.
   *
   * @return Validation flag.
   */
  bool IsValid() const;

  /**
   * @brief Returns the number of boolean semantic masks stored here.
   *
   * @return Boolean mask count.
   */
  int BoolMaskCount() const;
};

/**
 * @brief Builds semantic masks from raw level cells.
 *
 * @param level Raw loaded level data.
 * @param error Error text populated when building fails.
 * @return Built semantic masks. Returned masks are invalid on failure.
 */
SemanticMasks BuildSemanticMasks(const LevelData& level, std::string* error);

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_SEMANTIC_MASKS_H_
