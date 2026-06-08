#ifndef SHOOT_AND_RUN_CPP_SRC_WINDOW_WINDOW_STATE_H_
#define SHOOT_AND_RUN_CPP_SRC_WINDOW_WINDOW_STATE_H_

namespace sar {

/**
 * @brief Physical monitor bounds used to place the application window.
 */
struct MonitorInfo {
  int x = 0;
  int y = 0;
  int width = 1280;
  int height = 720;
};

/**
 * @brief Current window dimensions, position and resolved UI scale.
 */
struct WindowState {
  int monitor_width = 1280;
  int monitor_height = 720;
  int width = 1280;
  int height = 720;
  int x = 0;
  int y = 0;
  float ui_scale = 1.0F;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_WINDOW_WINDOW_STATE_H_
