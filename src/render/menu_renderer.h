#ifndef SHOOT_AND_RUN_CPP_SRC_RENDER_MENU_RENDERER_H_
#define SHOOT_AND_RUN_CPP_SRC_RENDER_MENU_RENDERER_H_

#include "ui/confirm_dialog.h"
#include "ui/main_menu.h"
#include "window/window_state.h"

namespace sar {

class MenuRenderer {
 public:
  /**
   * @brief Draws the main menu.
   *
   * @param menu Menu state to render.
   * @param window Current window state.
   */
  void DrawMainMenu(const MainMenu& menu, const WindowState& window) const;

  /**
   * @brief Draws a modal confirmation dialog.
   *
   * @param dialog Dialog state to render.
   * @param window Current window state.
   */
  void DrawConfirmDialog(const ConfirmDialog& dialog,
                         const WindowState& window) const;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_RENDER_MENU_RENDERER_H_
