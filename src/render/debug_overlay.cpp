/**
 * @file src/render/debug_overlay.cpp
 * @brief 2D/debug rendering helpers retained by the application shell. Contains implementation
 * for debug_overlay.cpp.
 */

#include "render/debug_overlay.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <string>

#include "platform/memory_info.h"

namespace sar {
namespace {

/**
 * @brief Returns screen name.
 */
const char* ScreenName(AppScreen screen) {
  switch (screen) {
    case AppScreen::kMainMenu:
      return "main_menu";
    case AppScreen::kMapPreparing:
      return "map_preparing";
    case AppScreen::kGame:
      return "game";
    case AppScreen::kSettings:
      return "settings";
  }

  return "unknown";
}

/**
 * @brief Executes the scaled font size operation.
 */
int ScaledFontSize(const UiFont& font, const WindowState& window,
                   float multiplier) {
  const float size = static_cast<float>(font.base_size()) * multiplier *
                     window.ui_scale;
  return std::max(1, static_cast<int>(std::lround(size)));
}

}  // namespace

/**
 * @brief Draws runtime visuals.
 */
void DebugOverlay::Draw(std::string_view version, AppScreen screen,
                        const WindowState& window, const UiFont& font,
                        const ServiceInfoOverlayData& service_info) const {
  const int font_size = ScaledFontSize(font, window, 0.65F);
  const int x = static_cast<int>(16.0F * window.ui_scale);
  int y = static_cast<int>(16.0F * window.ui_scale);
  const int line_step = font_size + static_cast<int>(5.0F * window.ui_scale);
  const Color color = Color{155, 160, 180, 255};

  font.DrawTextLine(std::string("version: ") + std::string(version), x, y,
                    font_size, color);
  y += line_step;
  font.DrawTextLine(std::string("screen: ") + ScreenName(screen), x, y,
                    font_size, color);
  y += line_step;
  font.DrawTextLine(std::string("window: ") + std::to_string(window.width) +
                        "x" + std::to_string(window.height),
                    x, y, font_size, color);
  y += line_step;
  font.DrawTextLine(TextFormat("ui_scale: %.2f", window.ui_scale), x, y,
                    font_size, color);

  if (!service_info.show_memory) {
    return;
  }

  y += line_step;
  const std::string memory_text =
      service_info.memory_available
          ? "memory: " + FormatMegabytes(service_info.resident_memory_bytes)
          : "memory: n/a";
  font.DrawTextLine(memory_text, x, y, font_size, color);
}

}  // namespace sar
