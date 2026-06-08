#ifndef SHOOT_AND_RUN_CPP_SRC_LEVEL_RUNTIME_OBJECT_H_
#define SHOOT_AND_RUN_CPP_SRC_LEVEL_RUNTIME_OBJECT_H_

/**
 * @file src/level/runtime_object.h
 * @brief Generated map package data contracts and loading logic. Contains public declarations
 * for runtime_object.h.
 */

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
  std::string id;  ///< Stable identifier loaded from source data or configuration.
  std::string type;  ///< Semantic type loaded from source data or configuration.
  std::string family;  ///< Higher-level object family used for grouping related objects.
  int x = 0;  ///< Tile, screen, or world coordinate for x.
  int y = 0;  ///< Tile, screen, or world coordinate for y.
  int width = 1;  ///< Size component for width.
  int height = 1;  ///< Signed elevation level for this tile or object.
  int rotation = 0;  ///< Rotation value carried by this data structure.
  std::int8_t elevation = 0;  ///< Elevation value carried by this data structure.
  bool blocks_movement = false;  ///< Blocks movement value carried by this data structure.
  bool blocks_projectiles = false;  ///< true when projectiles cannot pass through this cell.
  bool blocks_vision = false;  ///< true when line-of-sight cannot pass through this cell.
  std::vector<std::string> tags;  ///< Semantic tags attached to this entity.
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_LEVEL_RUNTIME_OBJECT_H_
