/**
 * @file src/render/menu_renderer.cpp
 * @brief 2D/debug rendering helpers retained by the application shell. Contains implementation
 * for menu_renderer.cpp.
 */

#include "render/menu_renderer.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "ui/ui_layout.h"

namespace sar {
namespace {

/**
 * @brief Converts to raylib rect.
 */
Rectangle ToRaylibRect(const Rect& rect) {
  return Rectangle{rect.x, rect.y, rect.width, rect.height};
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
 * @brief Draws main menu.
 */
void MenuRenderer::DrawMainMenu(const MainMenu& menu,
                                const WindowState& window,
                                const UiFont& font) const {
  const auto layouts = CalculateMainMenuLayout(
      static_cast<int>(menu.items().size()), window);
  const int font_size = ScaledFontSize(font, window, 1.0F);

  for (const MenuItemLayout& layout : layouts) {
    const MenuItem& item = menu.items()[static_cast<std::size_t>(layout.index)];
    const bool selected = layout.index == menu.selected_index();
    const Color fill_color = selected ? Color{56, 64, 86, 255}
                                      : Color{32, 34, 46, 255};
    const Color border_color = selected ? Color{190, 200, 230, 255}
                                        : Color{72, 76, 92, 255};
    const Color text_color = item.enabled ? Color{230, 230, 240, 255}
                                          : Color{110, 110, 120, 255};

    DrawRectangleRounded(ToRaylibRect(layout.bounds), 0.14F, 8, fill_color);
    DrawRectangleRoundedLinesEx(ToRaylibRect(layout.bounds), 0.14F, 8, 1.5F,
                                border_color);

    const int text_width = font.MeasureTextWidth(item.title, font_size);
    const int text_x = static_cast<int>(layout.bounds.x +
        (layout.bounds.width - static_cast<float>(text_width)) * 0.5F);
    const int text_y = static_cast<int>(layout.bounds.y +
        (layout.bounds.height - static_cast<float>(font_size)) * 0.5F);
    font.DrawTextLine(item.title, text_x, text_y, font_size, text_color);
  }
}

/**
 * @brief Draws confirm dialog.
 */
void MenuRenderer::DrawConfirmDialog(const ConfirmDialog& dialog,
                                     const WindowState& window,
                                     const UiFont& font) const {
  DrawRectangle(0, 0, window.width, window.height, Color{0, 0, 0, 150});

  const ConfirmDialogLayout layout = CalculateConfirmDialogLayout(window);
  DrawRectangleRounded(ToRaylibRect(layout.dialog_bounds), 0.08F, 12,
                       Color{28, 30, 42, 255});
  DrawRectangleRoundedLinesEx(ToRaylibRect(layout.dialog_bounds), 0.08F, 12,
                              1.5F, Color{160, 170, 205, 255});

  const int title_size = ScaledFontSize(font, window, 1.2F);
  const int message_size = ScaledFontSize(font, window, 0.82F);
  font.DrawTextLine(dialog.title(),
                    static_cast<int>(layout.dialog_bounds.x + 32.0F),
                    static_cast<int>(layout.dialog_bounds.y + 32.0F),
                    title_size, Color{235, 235, 245, 255});
  font.DrawTextLine(dialog.message(),
                    static_cast<int>(layout.dialog_bounds.x + 32.0F),
                    static_cast<int>(layout.dialog_bounds.y + 84.0F),
                    message_size, Color{190, 195, 210, 255});

  const bool yes_selected = dialog.selected_choice() == DialogChoice::kYes;
  const bool no_selected = dialog.selected_choice() == DialogChoice::kNo;
  const Color yes_fill = yes_selected ? Color{74, 68, 84, 255}
                                      : Color{42, 44, 58, 255};
  const Color no_fill = no_selected ? Color{74, 68, 84, 255}
                                    : Color{42, 44, 58, 255};

  DrawRectangleRounded(ToRaylibRect(layout.yes_bounds), 0.12F, 8, yes_fill);
  DrawRectangleRounded(ToRaylibRect(layout.no_bounds), 0.12F, 8, no_fill);
  DrawRectangleRoundedLinesEx(ToRaylibRect(layout.yes_bounds), 0.12F, 8, 1.2F,
                              Color{170, 170, 195, 255});
  DrawRectangleRoundedLinesEx(ToRaylibRect(layout.no_bounds), 0.12F, 8, 1.2F,
                              Color{170, 170, 195, 255});

  const int button_size = ScaledFontSize(font, window, 0.9F);
  font.DrawTextLine("Yes",
                    static_cast<int>(layout.yes_bounds.x +
                                     40.0F * window.ui_scale),
                    static_cast<int>(layout.yes_bounds.y +
                                     9.0F * window.ui_scale),
                    button_size, Color{230, 230, 240, 255});
  font.DrawTextLine("No",
                    static_cast<int>(layout.no_bounds.x +
                                     46.0F * window.ui_scale),
                    static_cast<int>(layout.no_bounds.y +
                                     9.0F * window.ui_scale),
                    button_size, Color{230, 230, 240, 255});
}

}  // namespace sar
