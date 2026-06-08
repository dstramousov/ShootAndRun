#ifndef SHOOT_AND_RUN_CPP_SRC_LEVEL_MARKER_H_
#define SHOOT_AND_RUN_CPP_SRC_LEVEL_MARKER_H_

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
  std::string id;
  std::string type;
  int x = 0;
  int y = 0;
  std::int8_t elevation = 0;
  std::vector<std::string> tags;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_LEVEL_MARKER_H_
