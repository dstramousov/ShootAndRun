#ifndef SHOOT_AND_RUN_CPP_SRC_WINDOW_WINDOW_STATE_H_
#define SHOOT_AND_RUN_CPP_SRC_WINDOW_WINDOW_STATE_H_

/**
 * @file src/window/window_state.h
 * @brief Window configuration, runtime state, and layout calculation. Contains public
 * declarations for window_state.h.
 */

namespace sar {

/**
 * @brief Physical monitor bounds used to place the application window.
 */
struct MonitorInfo {
  int x = 0;  ///< Tile, screen, or world coordinate for x.
  int y = 0;  ///< Tile, screen, or world coordinate for y.
  int width = 1280;  ///< Size component for width.
  int height = 720;  ///< Signed elevation level for this tile or object.
};

/**
 * @brief Current window dimensions, position and resolved UI scale.
 */
struct WindowState {
  int monitor_width = 1280;  ///< Size component for monitor width.
  int monitor_height = 720;  ///< Size component for monitor height.
  int width = 1280;  ///< Size component for width.
  int height = 720;  ///< Signed elevation level for this tile or object.
  int x = 0;  ///< Tile, screen, or world coordinate for x.
  int y = 0;  ///< Tile, screen, or world coordinate for y.
  float ui_scale = 1.0F;  ///< Scaling factor for UI scale.
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_WINDOW_WINDOW_STATE_H_
