#ifndef SHOOT_AND_RUN_CPP_SRC_LEVEL_RUNTIME_GRID_H_
#define SHOOT_AND_RUN_CPP_SRC_LEVEL_RUNTIME_GRID_H_

/**
 * @file src/level/runtime_grid.h
 * @brief Generated map package data contracts and loading logic. Contains public declarations
 * for runtime_grid.h.
 */

#include <cstdint>

#include "level/terrain_type.h"

namespace sar {

/**
 * @brief Runtime gameplay properties for one map tile.
 *
 * Runtime grids are the source of truth for movement, collision, visibility
 * blocking and elevation. Rendering code may visualize these values, but it
 * must not reinterpret them as decorative-only data.
 */
struct RuntimeCell {
  TerrainType terrain = TerrainType::kUnknown;  ///< Base terrain classification for this runtime cell.
  bool walkable = false;  ///< true when movement is allowed by the movement grid.
  bool collision = false;  ///< true when this cell blocks physical movement.
  bool blocks_projectiles = false;  ///< true when projectiles cannot pass through this cell.
  bool blocks_vision = false;  ///< true when line-of-sight cannot pass through this cell.
  std::uint8_t cover = 0;  ///< Cover strength encoded by the runtime grid.
  std::uint8_t concealment = 0;  ///< Concealment strength encoded by the runtime grid.
  std::int8_t height = 0;  ///< Signed elevation level for this tile or object.
  float movement_multiplier = 1.0F;  ///< Movement speed multiplier applied on this tile.
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_LEVEL_RUNTIME_GRID_H_
