#ifndef SHOOT_AND_RUN_CPP_SRC_LEVEL_GAMEPLAY_ZONE_H_
#define SHOOT_AND_RUN_CPP_SRC_LEVEL_GAMEPLAY_ZONE_H_

/**
 * @file src/level/gameplay_zone.h
 * @brief Generated map package data contracts and loading logic. Contains public declarations
 * for gameplay_zone.h.
 */

#include <string>
#include <vector>

namespace sar {

/**
 * @brief Gameplay area used by spawning, encounters and high-level logic.
 */
struct GameplayZone {
  std::string id;  ///< Stable identifier loaded from source data or configuration.
  std::string type;  ///< Semantic type loaded from source data or configuration.
  std::string shape;  ///< Shape value carried by this data structure.
  int x = 0;  ///< Tile, screen, or world coordinate for x.
  int y = 0;  ///< Tile, screen, or world coordinate for y.
  int width = 0;  ///< Size component for width.
  int height = 0;  ///< Signed elevation level for this tile or object.
  int radius = 0;  ///< Size component for radius.
  std::vector<std::string> tags;  ///< Semantic tags attached to this entity.
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_LEVEL_GAMEPLAY_ZONE_H_
