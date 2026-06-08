#ifndef SHOOT_AND_RUN_CPP_SRC_CORE_TYPES_H_
#define SHOOT_AND_RUN_CPP_SRC_CORE_TYPES_H_

namespace sar {

/**
 * @brief Two-dimensional floating point vector.
 *
 * Used for screen-space and lightweight world-space values that do not need a
 * dependency on raylib types in public headers.
 */
struct Vec2 {
  float x = 0.0F;
  float y = 0.0F;
};

/**
 * @brief Axis-aligned rectangle in floating point coordinates.
 *
 * The rectangle stores the top-left corner and dimensions. It is used by UI
 * layout code for hit testing and drawing bounds.
 */
struct Rect {
  float x = 0.0F;
  float y = 0.0F;
  float width = 0.0F;
  float height = 0.0F;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_CORE_TYPES_H_
