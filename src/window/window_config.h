#ifndef SHOOT_AND_RUN_CPP_SRC_WINDOW_WINDOW_CONFIG_H_
#define SHOOT_AND_RUN_CPP_SRC_WINDOW_WINDOW_CONFIG_H_

/**
 * @file src/window/window_config.h
 * @brief Window configuration, runtime state, and layout calculation. Contains public
 * declarations for window_config.h.
 */

namespace sar {

/**
 * @brief Window sizing and UI scaling preferences loaded before raylib startup.
 */
struct WindowConfig {
  int preferred_width = 1600;  ///< Preferred window width in pixels before monitor clamping.
  int preferred_height = 900;  ///< Preferred window height in pixels before monitor clamping.
  int fallback_width = 1280;  ///< Fallback window width used on smaller monitors.
  int fallback_height = 720;  ///< Fallback window height used on smaller monitors.
  int ui_reference_width = 1280;  ///< Reference UI width used to compute UI scale.
  int ui_reference_height = 720;  ///< Reference UI height used to compute UI scale.
  float max_monitor_fraction = 0.90F;  ///< Maximum fraction of the monitor that the window may occupy.
  float ui_scale_min = 0.75F;  ///< Minimum allowed UI scale.
  float ui_scale_max = 2.00F;  ///< Maximum allowed UI scale.
  bool resizable = true;  ///< true when the raylib window may be resized.
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_WINDOW_WINDOW_CONFIG_H_
