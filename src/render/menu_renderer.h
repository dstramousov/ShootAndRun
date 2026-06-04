#ifndef SHOOT_AND_RUN_CPP_SRC_RENDER_MENU_RENDERER_H_
#define SHOOT_AND_RUN_CPP_SRC_RENDER_MENU_RENDERER_H_

#include "render/ui_font.h"
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
   * @param font UI font resource.
   */
  void DrawMainMenu(const MainMenu& menu, const WindowState& window,
                    const UiFont& font) const;

  /**
   * @brief Draws a modal confirmation dialog.
   *
   * @param dialog Dialog state to render.
   * @param window Current window state.
   * @param font UI font resource.
   */
  void DrawConfirmDialog(const ConfirmDialog& dialog,
                         const WindowState& window,
                         const UiFont& font) const;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_RENDER_MENU_RENDERER_H_
