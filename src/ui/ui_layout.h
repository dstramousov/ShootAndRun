#ifndef SHOOT_AND_RUN_CPP_SRC_UI_UI_LAYOUT_H_
#define SHOOT_AND_RUN_CPP_SRC_UI_UI_LAYOUT_H_

/**
 * @file src/ui/ui_layout.h
 * @brief Main menu, confirmation dialog, and UI layout. Contains public declarations for
 * ui_layout.h.
 */

#include <vector>

#include "core/types.h"
#include "window/window_state.h"

namespace sar {

/**
 * @brief Stores menu item layout data shared between runtime systems.
 */
struct MenuItemLayout {
  int index = -1;  ///< Index value carried by this data structure.
  Rect bounds;  ///< Bounds value carried by this data structure.
};

/**
 * @brief Stores confirm dialog layout data shared between runtime systems.
 */
struct ConfirmDialogLayout {
  Rect dialog_bounds;  ///< Dialog bounds value carried by this data structure.
  Rect yes_bounds;  ///< Yes bounds value carried by this data structure.
  Rect no_bounds;  ///< No bounds value carried by this data structure.
};

/**
 * @brief Calculates clickable rectangles for main menu items.
 *
 * @param item_count Number of menu items.
 * @param window Current window state.
 * @return Layout rectangles for each menu item.
 */
std::vector<MenuItemLayout> CalculateMainMenuLayout(int item_count,
                                                    const WindowState& window);

/**
 * @brief Calculates confirmation dialog layout rectangles.
 *
 * @param window Current window state.
 * @return Dialog and button rectangles.
 */
ConfirmDialogLayout CalculateConfirmDialogLayout(const WindowState& window);

/**
 * @brief Finds a menu item at a screen position.
 *
 * @param layouts Precomputed menu item layouts.
 * @param position Screen position in pixels.
 * @return Item index or -1 when no item was hit.
 */
int HitTestMenuItem(const std::vector<MenuItemLayout>& layouts,
                    const Vec2& position);

/**
 * @brief Checks whether a point is inside a rectangle.
 *
 * @param rect Rectangle to test.
 * @param position Screen position in pixels.
 * @return true if the point is inside the rectangle.
 */
bool ContainsPoint(const Rect& rect, const Vec2& position);

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_UI_UI_LAYOUT_H_
