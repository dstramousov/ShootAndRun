#ifndef SHOOT_AND_RUN_CPP_SRC_WINDOW_WINDOW_CONFIG_H_
#define SHOOT_AND_RUN_CPP_SRC_WINDOW_WINDOW_CONFIG_H_

namespace sar {

struct WindowConfig {
  int base_width = 1280;
  int base_height = 720;
  int fallback_width = 960;
  int fallback_height = 540;
  float max_monitor_fraction = 0.90F;
  float ui_scale_min = 0.75F;
  float ui_scale_max = 2.00F;
  bool resizable = true;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_WINDOW_WINDOW_CONFIG_H_
