#ifndef SHOOT_AND_RUN_CPP_SRC_LEVEL_RUNTIME_OBJECT_H_
#define SHOOT_AND_RUN_CPP_SRC_LEVEL_RUNTIME_OBJECT_H_

#include <cstdint>
#include <string>
#include <vector>

namespace sar {

/**
 * @brief Runtime object footprint and metadata loaded from the map package.
 *
 * Object coordinates and dimensions are stored in tile units. Runtime grids are
 * still treated as the gameplay source of truth for collision and visibility.
 */
struct RuntimeObject {
  std::string id;
  std::string type;
  std::string family;
  int x = 0;
  int y = 0;
  int width = 1;
  int height = 1;
  int rotation = 0;
  std::int8_t elevation = 0;
  bool blocks_movement = false;
  bool blocks_projectiles = false;
  bool blocks_vision = false;
  std::vector<std::string> tags;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_LEVEL_RUNTIME_OBJECT_H_
