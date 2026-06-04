#include "render/renderer.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace sar {
namespace {

int ScaledFontSize(const UiFont& font, const WindowState& window,
                   float multiplier) {
  const float size = static_cast<float>(font.base_size()) * multiplier *
                     window.ui_scale;
  return std::max(1, static_cast<int>(std::lround(size)));
}

}  // namespace

void Renderer::BeginFrame() const {
  BeginDrawing();
  ClearBackground(Color{18, 18, 24, 255});
}

void Renderer::EndFrame() const { EndDrawing(); }

void Renderer::DrawFps(const WindowState& window, const UiFont& font) const {
  const int font_size = ScaledFontSize(font, window, 0.8F);
  const int padding = static_cast<int>(16.0F * window.ui_scale);
  const std::string text = std::string("FPS: ") + std::to_string(GetFPS());
  const int text_width = font.MeasureTextWidth(text, font_size);
  font.DrawTextLine(text, window.width - text_width - padding, padding,
                    font_size, Color{210, 210, 220, 255});
}

void Renderer::DrawTitle(std::string_view title, const WindowState& window,
                         const UiFont& font) const {
  const int font_size = ScaledFontSize(font, window, 2.0F);
  const int text_width = font.MeasureTextWidth(title, font_size);
  font.DrawTextLine(title, (window.width - text_width) / 2,
                    static_cast<int>(72.0F * window.ui_scale), font_size,
                    Color{230, 230, 240, 255});
}

}  // namespace sar
