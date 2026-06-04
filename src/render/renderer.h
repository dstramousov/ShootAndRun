#ifndef SHOOT_AND_RUN_CPP_SRC_RENDER_RENDERER_H_
#define SHOOT_AND_RUN_CPP_SRC_RENDER_RENDERER_H_

#include <string_view>

#include "app/app_config.h"
#include "window/window_state.h"

namespace sar {

class Renderer {
 public:
  /**
   * @brief Starts a new raylib frame and clears the background.
   */
  void BeginFrame() const;

  /**
   * @brief Finishes the current raylib frame.
   */
  void EndFrame() const;

  /**
   * @brief Draws the current FPS counter in the top-right corner.
   *
   * @param window Current window state.
   */
  void DrawFps(const WindowState& window) const;

  /**
   * @brief Draws the application title.
   *
   * @param title Title text.
   * @param window Current window state.
   */
  void DrawTitle(std::string_view title, const WindowState& window) const;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_RENDER_RENDERER_H_
