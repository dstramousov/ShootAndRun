#ifndef SHOOT_AND_RUN_CPP_SRC_LEVEL_PLACE_H_
#define SHOOT_AND_RUN_CPP_SRC_LEVEL_PLACE_H_

/**
 * @file src/level/place.h
 * @brief Generated map package data contracts and loading logic. Contains public declarations
 * for place.h.
 */

#include <string>
#include <vector>

namespace sar {

/**
 * @brief Named semantic area or point of interest from the level package.
 */
struct Place {
  std::string id;  ///< Stable identifier loaded from source data or configuration.
  std::string type;  ///< Semantic type loaded from source data or configuration.
  std::string name;  ///< Human-readable name or configuration key.
  int x = 0;  ///< Tile, screen, or world coordinate for x.
  int y = 0;  ///< Tile, screen, or world coordinate for y.
  int radius = 0;  ///< Size component for radius.
  std::vector<std::string> tags;  ///< Semantic tags attached to this entity.
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_LEVEL_PLACE_H_
