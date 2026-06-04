#include "render/renderer.h"

#include <raylib.h>

#include <string>

namespace sar {

void Renderer::BeginFrame() const {
  BeginDrawing();
  ClearBackground(Color{18, 18, 24, 255});
}

void Renderer::EndFrame() const { EndDrawing(); }

void Renderer::DrawFps(const WindowState& window, const UiFont& font) const {
  const int font_size = static_cast<int>(18.0F * window.ui_scale);
  const int padding = static_cast<int>(16.0F * window.ui_scale);
  const std::string text = std::string("FPS: ") + std::to_string(GetFPS());
  const int text_width = font.MeasureTextWidth(text, font_size);
  font.DrawTextLine(text, window.width - text_width - padding, padding,
                    font_size, Color{210, 210, 220, 255});
}

void Renderer::DrawTitle(std::string_view title, const WindowState& window,
                         const UiFont& font) const {
  const int font_size = static_cast<int>(40.0F * window.ui_scale);
  const int text_width = font.MeasureTextWidth(title, font_size);
  font.DrawTextLine(title, (window.width - text_width) / 2,
                    static_cast<int>(72.0F * window.ui_scale), font_size,
                    Color{230, 230, 240, 255});
}

}  // namespace sar
