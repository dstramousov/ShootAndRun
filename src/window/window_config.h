#ifndef SHOOT_AND_RUN_CPP_SRC_WINDOW_WINDOW_CONFIG_H_
#define SHOOT_AND_RUN_CPP_SRC_WINDOW_WINDOW_CONFIG_H_

namespace sar {

/**
 * @brief Window sizing and UI scaling preferences loaded before raylib startup.
 */
struct WindowConfig {
  int preferred_width = 1600;
  int preferred_height = 900;
  int fallback_width = 1280;
  int fallback_height = 720;
  int ui_reference_width = 1280;
  int ui_reference_height = 720;
  float max_monitor_fraction = 0.90F;
  float ui_scale_min = 0.75F;
  float ui_scale_max = 2.00F;
  bool resizable = true;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_WINDOW_WINDOW_CONFIG_H_
