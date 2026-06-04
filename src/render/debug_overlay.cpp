#include "render/debug_overlay.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace sar {
namespace {

const char* ScreenName(AppScreen screen) {
  switch (screen) {
    case AppScreen::kMainMenu:
      return "main_menu";
    case AppScreen::kGame:
      return "game";
    case AppScreen::kSettings:
      return "settings";
  }

  return "unknown";
}

int ScaledFontSize(const UiFont& font, const WindowState& window,
                   float multiplier) {
  const float size = static_cast<float>(font.base_size()) * multiplier *
                     window.ui_scale;
  return std::max(1, static_cast<int>(std::lround(size)));
}

}  // namespace

void DebugOverlay::Draw(std::string_view version, AppScreen screen,
                        const WindowState& window,
                        const UiFont& font) const {
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
}

}  // namespace sar
