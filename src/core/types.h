#ifndef SHOOT_AND_RUN_CPP_SRC_CORE_TYPES_H_
#define SHOOT_AND_RUN_CPP_SRC_CORE_TYPES_H_

/**
 * @file src/core/types.h
 * @brief Small shared value types used across independent subsystems. Contains public
 * declarations for types.h.
 */

namespace sar {

/**
 * @brief Two-dimensional floating point vector.
 *
 * Used for screen-space and lightweight world-space values that do not need a
 * dependency on raylib types in public headers.
 */
struct Vec2 {
  float x = 0.0F;  ///< Tile, screen, or world coordinate for x.
  float y = 0.0F;  ///< Tile, screen, or world coordinate for y.
};

/**
 * @brief Axis-aligned rectangle in floating point coordinates.
 *
 * The rectangle stores the top-left corner and dimensions. It is used by UI
 * layout code for hit testing and drawing bounds.
 */
struct Rect {
  float x = 0.0F;  ///< Tile, screen, or world coordinate for x.
  float y = 0.0F;  ///< Tile, screen, or world coordinate for y.
  float width = 0.0F;  ///< Size component for width.
  float height = 0.0F;  ///< Signed elevation level for this tile or object.
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_CORE_TYPES_H_
