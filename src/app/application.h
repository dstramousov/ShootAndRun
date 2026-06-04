#ifndef SHOOT_AND_RUN_CPP_SRC_APP_APPLICATION_H_
#define SHOOT_AND_RUN_CPP_SRC_APP_APPLICATION_H_

#include <optional>

#include "app/app_config.h"
#include "app/app_state.h"
#include "game/game_session.h"
#include "input/input_system.h"
#include "logging/logger.h"
#include "render/debug_overlay.h"
#include "render/menu_renderer.h"
#include "render/renderer.h"
#include "ui/confirm_dialog.h"
#include "ui/main_menu.h"
#include "window/window_state.h"

namespace sar {

class Application {
 public:
  /**
   * @brief Creates an application instance.
   *
   * @param config Runtime application configuration.
   */
  explicit Application(AppConfig config);

  /**
   * @brief Runs the application main loop.
   *
   * @return Process exit code.
   */
  int Run();

 private:
  void InitializeWindow();
  void ShutdownWindow();
  void UpdateWindowStateFromRaylib();
  void LogStartup();
  void HandleInput(const InputState& input);
  void HandleDialogInput(const InputState& input);
  void HandleMenuInput(const InputState& input);
  void HandleGameInput(const InputState& input);
  void ActivateMenuItem(const MenuItem& item);
  void OpenExitDialog();
  void RenderFrame();

  AppConfig config_;
  Logger logger_;
  WindowState window_state_;
  AppScreen screen_ = AppScreen::kMainMenu;
  bool exit_requested_ = false;
  bool window_initialized_ = false;
  InputSystem input_system_;
  Renderer renderer_;
  MenuRenderer menu_renderer_;
  DebugOverlay debug_overlay_;
  MainMenu main_menu_;
  GameSession game_session_;
  std::optional<ConfirmDialog> confirm_dialog_;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_APP_APPLICATION_H_
