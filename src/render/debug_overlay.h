#ifndef SHOOT_AND_RUN_CPP_SRC_RENDER_DEBUG_OVERLAY_H_
#define SHOOT_AND_RUN_CPP_SRC_RENDER_DEBUG_OVERLAY_H_

#include <string_view>

#include "app/app_state.h"
#include "render/ui_font.h"
#include "window/window_state.h"

namespace sar {

class DebugOverlay {
 public:
  /**
   * @brief Draws debug information on the screen.
   *
   * @param version Application version string.
   * @param screen Current application screen.
   * @param window Current window state.
   * @param font UI font resource.
   */
  void Draw(std::string_view version, AppScreen screen,
            const WindowState& window, const UiFont& font) const;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_RENDER_DEBUG_OVERLAY_H_
