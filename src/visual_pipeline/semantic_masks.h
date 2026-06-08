#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_SEMANTIC_MASKS_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_SEMANTIC_MASKS_H_

#include <cstdint>
#include <string>
#include <vector>

#include "level/level_data.h"

namespace sar::visual_pipeline {

/**
 * @brief Counters describing semantic mask coverage for one level.
 */
struct SemanticMaskSummary {
  int total_tiles = 0;
  int open_ground_tiles = 0;
  int forest_tiles = 0;
  int road_tiles = 0;
  int swamp_tiles = 0;
  int water_tiles = 0;
  int ruins_tiles = 0;
  int wall_tiles = 0;
  int unknown_tiles = 0;
  int walkable_tiles = 0;
  int blocked_tiles = 0;
  int vision_blocked_tiles = 0;
  int projectile_blocked_tiles = 0;
  int cover_tiles = 0;
  int concealment_tiles = 0;
  int low_ground_tiles = 0;
  int elevated_tiles = 0;

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
  LevelSize size;
  std::vector<std::uint8_t> open_ground;
  std::vector<std::uint8_t> forest;
  std::vector<std::uint8_t> road;
  std::vector<std::uint8_t> swamp;
  std::vector<std::uint8_t> water;
  std::vector<std::uint8_t> ruins;
  std::vector<std::uint8_t> wall;
  std::vector<std::uint8_t> unknown;
  std::vector<std::uint8_t> walkable;
  std::vector<std::uint8_t> blocked;
  std::vector<std::uint8_t> vision_blocked;
  std::vector<std::uint8_t> projectile_blocked;
  std::vector<std::uint8_t> cover;
  std::vector<std::uint8_t> concealment;
  std::vector<std::int8_t> height;
  SemanticMaskSummary summary;

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
