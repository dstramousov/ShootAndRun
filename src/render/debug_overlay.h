#ifndef SHOOT_AND_RUN_CPP_SRC_RENDER_DEBUG_OVERLAY_H_
#define SHOOT_AND_RUN_CPP_SRC_RENDER_DEBUG_OVERLAY_H_

#include <cstdint>
#include <string_view>

#include "app/app_state.h"
#include "render/ui_font.h"
#include "window/window_state.h"

namespace sar {

struct ServiceInfoOverlayData {
  bool show_memory = false;
  bool memory_available = false;
  std::uint64_t resident_memory_bytes = 0;
};

class DebugOverlay {
 public:
  /**
   * @brief Draws debug information on the screen.
   *
   * @param version Application version string.
   * @param screen Current application screen.
   * @param window Current window state.
   * @param font UI font resource.
   * @param service_info Service information values to display.
   */
  void Draw(std::string_view version, AppScreen screen,
            const WindowState& window, const UiFont& font,
            const ServiceInfoOverlayData& service_info) const;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_RENDER_DEBUG_OVERLAY_H_
