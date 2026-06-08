#ifndef SHOOT_AND_RUN_CPP_SRC_LEVEL_MARKER_H_
#define SHOOT_AND_RUN_CPP_SRC_LEVEL_MARKER_H_

/**
 * @file src/level/marker.h
 * @brief Generated map package data contracts and loading logic. Contains public declarations
 * for marker.h.
 */

#include <cstdint>
#include <string>
#include <vector>

namespace sar {

/**
 * @brief Gameplay marker loaded from the map package.
 *
 * Markers represent concrete runtime points such as spawn positions, exits or
 * interaction anchors. Coordinates are stored in tile units.
 */
struct Marker {
  std::string id;  ///< Stable identifier loaded from source data or configuration.
  std::string type;  ///< Semantic type loaded from source data or configuration.
  int x = 0;  ///< Tile, screen, or world coordinate for x.
  int y = 0;  ///< Tile, screen, or world coordinate for y.
  std::int8_t elevation = 0;  ///< Elevation value carried by this data structure.
  std::vector<std::string> tags;  ///< Semantic tags attached to this entity.
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_LEVEL_MARKER_H_
