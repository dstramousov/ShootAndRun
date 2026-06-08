#ifndef SHOOT_AND_RUN_CPP_SRC_LEVEL_RUNTIME_GRID_H_
#define SHOOT_AND_RUN_CPP_SRC_LEVEL_RUNTIME_GRID_H_

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
  TerrainType terrain = TerrainType::kUnknown;
  bool walkable = false;
  bool collision = false;
  bool blocks_projectiles = false;
  bool blocks_vision = false;
  std::uint8_t cover = 0;
  std::uint8_t concealment = 0;
  std::int8_t height = 0;
  float movement_multiplier = 1.0F;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_LEVEL_RUNTIME_GRID_H_
