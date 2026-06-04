#include "window/window_layout.h"

#include <algorithm>

namespace sar {

WindowState CalculateWindowState(const MonitorInfo& monitor,
                                 const WindowConfig& config) {
  const int max_width = static_cast<int>(
      static_cast<float>(monitor.width) * config.max_monitor_fraction);
  const int max_height = static_cast<int>(
      static_cast<float>(monitor.height) * config.max_monitor_fraction);

  int window_width = config.base_width;
  int window_height = config.base_height;

  if (window_width > max_width || window_height > max_height) {
    window_width = config.fallback_width;
    window_height = config.fallback_height;
  }

  window_width = std::max(320, std::min(window_width, max_width));
  window_height = std::max(240, std::min(window_height, max_height));

  WindowState state;
  state.monitor_width = monitor.width;
  state.monitor_height = monitor.height;
  state.width = window_width;
  state.height = window_height;
  state.x = monitor.x + (monitor.width - window_width) / 2;
  state.y = monitor.y + (monitor.height - window_height) / 2;
  state.ui_scale = CalculateUiScale(window_width, window_height, config);
  return state;
}

float CalculateUiScale(int window_width, int window_height,
                       const WindowConfig& config) {
  const float scale_x = static_cast<float>(window_width) /
                        static_cast<float>(config.base_width);
  const float scale_y = static_cast<float>(window_height) /
                        static_cast<float>(config.base_height);
  return std::clamp(std::min(scale_x, scale_y), config.ui_scale_min,
                    config.ui_scale_max);
}

}  // namespace sar
