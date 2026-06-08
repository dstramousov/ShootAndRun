/**
 * @file src/ui/ui_layout.cpp
 * @brief Main menu, confirmation dialog, and UI layout. Contains implementation for
 * ui_layout.cpp.
 */

#include "ui/ui_layout.h"

#include <algorithm>

namespace sar {

/**
 * @brief Executes the calculate main menu layout operation.
 */
std::vector<MenuItemLayout> CalculateMainMenuLayout(int item_count,
                                                    const WindowState& window) {
  std::vector<MenuItemLayout> layouts;
  if (item_count <= 0) {
    return layouts;
  }

  const float item_width = 320.0F * window.ui_scale;
  const float item_height = 44.0F * window.ui_scale;
  const float item_spacing = 12.0F * window.ui_scale;
  const float total_height =
      static_cast<float>(item_count) * item_height +
      static_cast<float>(std::max(0, item_count - 1)) * item_spacing;
  const float start_x = (static_cast<float>(window.width) - item_width) * 0.5F;
  const float start_y = (static_cast<float>(window.height) - total_height) * 0.5F;

  layouts.reserve(static_cast<std::size_t>(item_count));
  for (int i = 0; i < item_count; ++i) {
    const float y = start_y + static_cast<float>(i) * (item_height + item_spacing);
    layouts.push_back(MenuItemLayout{i, Rect{start_x, y, item_width, item_height}});
  }

  return layouts;
}

/**
 * @brief Executes the calculate confirm dialog layout operation.
 */
ConfirmDialogLayout CalculateConfirmDialogLayout(const WindowState& window) {
  const float dialog_width = 520.0F * window.ui_scale;
  const float dialog_height = 240.0F * window.ui_scale;
  const float button_width = 120.0F * window.ui_scale;
  const float button_height = 40.0F * window.ui_scale;
  const float button_gap = 24.0F * window.ui_scale;

  const float dialog_x = (static_cast<float>(window.width) - dialog_width) * 0.5F;
  const float dialog_y = (static_cast<float>(window.height) - dialog_height) * 0.5F;
  const float buttons_y = dialog_y + dialog_height - 72.0F * window.ui_scale;
  const float yes_x = dialog_x + dialog_width * 0.5F - button_width - button_gap * 0.5F;
  const float no_x = dialog_x + dialog_width * 0.5F + button_gap * 0.5F;

  return ConfirmDialogLayout{
      Rect{dialog_x, dialog_y, dialog_width, dialog_height},
      Rect{yes_x, buttons_y, button_width, button_height},
      Rect{no_x, buttons_y, button_width, button_height}};
}

/**
 * @brief Executes the hit test menu item operation.
 */
int HitTestMenuItem(const std::vector<MenuItemLayout>& layouts,
                    const Vec2& position) {
  for (const MenuItemLayout& layout : layouts) {
    if (ContainsPoint(layout.bounds, position)) {
      return layout.index;
    }
  }

  return -1;
}

/**
 * @brief Checks whether the container includes point.
 */
bool ContainsPoint(const Rect& rect, const Vec2& position) {
  return position.x >= rect.x && position.x <= rect.x + rect.width &&
         position.y >= rect.y && position.y <= rect.y + rect.height;
}

}  // namespace sar
