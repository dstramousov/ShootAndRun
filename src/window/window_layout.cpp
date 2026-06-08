/**
 * @file src/window/window_layout.cpp
 * @brief Window configuration, runtime state, and layout calculation. Contains implementation
 * for window_layout.cpp.
 */

#include "window/window_layout.h"

#include <algorithm>
#include <cmath>

namespace sar {

/**
 * @brief Executes the calculate window state operation.
 */
WindowState CalculateWindowState(const MonitorInfo& monitor,
                                 const WindowConfig& config) {
  const int max_width = static_cast<int>(
      std::floor(static_cast<float>(monitor.width) *
                 config.max_monitor_fraction));
  const int max_height = static_cast<int>(
      std::floor(static_cast<float>(monitor.height) *
                 config.max_monitor_fraction));

  int window_width = config.preferred_width;
  int window_height = config.preferred_height;

  if (window_width > max_width || window_height > max_height) {
    const float scale_x = static_cast<float>(max_width) /
                          static_cast<float>(std::max(1, window_width));
    const float scale_y = static_cast<float>(max_height) /
                          static_cast<float>(std::max(1, window_height));
    const float scale = std::min(scale_x, scale_y);
    window_width = static_cast<int>(
        std::floor(static_cast<float>(window_width) * scale));
    window_height = static_cast<int>(
        std::floor(static_cast<float>(window_height) * scale));
  }

  if (window_width < config.fallback_width ||
      window_height < config.fallback_height) {
    window_width = std::min(config.fallback_width, max_width);
    window_height = std::min(config.fallback_height, max_height);
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

/**
 * @brief Executes the calculate UI scale operation.
 */
float CalculateUiScale(int window_width, int window_height,
                       const WindowConfig& config) {
  const float scale_x = static_cast<float>(window_width) /
                        static_cast<float>(config.ui_reference_width);
  const float scale_y = static_cast<float>(window_height) /
                        static_cast<float>(config.ui_reference_height);
  return std::clamp(std::min(scale_x, scale_y), config.ui_scale_min,
                    config.ui_scale_max);
}

}  // namespace sar
