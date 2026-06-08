#ifndef SHOOT_AND_RUN_CPP_SRC_WINDOW_WINDOW_LAYOUT_H_
#define SHOOT_AND_RUN_CPP_SRC_WINDOW_WINDOW_LAYOUT_H_

/**
 * @file src/window/window_layout.h
 * @brief Window configuration, runtime state, and layout calculation. Contains public
 * declarations for window_layout.h.
 */

#include "window/window_config.h"
#include "window/window_state.h"

namespace sar {

/**
 * @brief Calculates a safe centered window state for a monitor.
 *
 * @param monitor Current monitor geometry.
 * @param config Window configuration limits.
 * @return Calculated window state.
 */
WindowState CalculateWindowState(const MonitorInfo& monitor,
                                 const WindowConfig& config);

/**
 * @brief Calculates UI scale for a concrete window size.
 *
 * @param window_width Current window width in pixels.
 * @param window_height Current window height in pixels.
 * @param config Window configuration limits.
 * @return Clamped UI scale value.
 */
float CalculateUiScale(int window_width, int window_height,
                       const WindowConfig& config);

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_WINDOW_WINDOW_LAYOUT_H_
