#ifndef SHOOT_AND_RUN_CPP_SRC_APP_APPLICATION_H_
#define SHOOT_AND_RUN_CPP_SRC_APP_APPLICATION_H_

#include <optional>

#include "app/app_config.h"
#include "app/app_state.h"
#include "app/project_config.h"
#include "game/game_session.h"
#include "input/input_system.h"
#include "level/level_loader.h"
#include "logging/logger.h"
#include "render/debug_overlay.h"
#include "render/level_renderer.h"
#include "render/menu_renderer.h"
#include "render/renderer.h"
#include "render/ui_font.h"
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
  void LoadProjectConfigAtStartup();
  void InitializeWindow();
  void LoadUiFont();
  void ShutdownWindow();
  void UpdateWindowStateFromRaylib();
  void LogStartup();
  void ApplyRaylibLogLevel();
  void UpdateServiceInfo();
  void ApplyFramePacing();
  void HandleInput(const InputState& input);
  void HandleDialogInput(const InputState& input);
  void HandleMenuInput(const InputState& input);
  void HandleGameInput(const InputState& input);
  void UpdateGameCamera(const InputState& input);
  void DrawGameOverlay() const;
  void ActivateMenuItem(const MenuItem& item);
  bool StartNewGameFromConfig();
  bool ValidateMapPackagePath(const ProjectConfig& project_config);
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
  LevelRenderer level_renderer_;
  MenuRenderer menu_renderer_;
  DebugOverlay debug_overlay_;
  MainMenu main_menu_;
  GameSession game_session_;
  LevelLoader level_loader_;
  UiFont ui_font_;
  std::optional<ProjectConfig> project_config_;
  std::optional<LevelPackageSummary> loaded_level_summary_;
  std::optional<LevelData> loaded_level_;
  LevelViewState level_view_;
  ServiceInfoOverlayData service_info_data_;
  double last_service_info_update_time_ = -1.0;
  std::optional<ConfirmDialog> confirm_dialog_;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_APP_APPLICATION_H_
