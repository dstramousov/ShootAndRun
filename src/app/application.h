#ifndef SHOOT_AND_RUN_CPP_SRC_APP_APPLICATION_H_
#define SHOOT_AND_RUN_CPP_SRC_APP_APPLICATION_H_

#include <optional>

#include <raylib.h>

#include "app/app_config.h"
#include "app/app_state.h"
#include "app/project_config.h"
#include "developer/developer_config.h"
#include "game/game_session.h"
#include "input/input_system.h"
#include "level/level_loader.h"
#include "logging/logger.h"
#include "render/debug_overlay.h"
#include "render/level_renderer.h"
#include "render/menu_renderer.h"
#include "render/renderer.h"
#include "render/ui_font.h"
#include "render3d/level_3d_renderer.h"
#include "ui/confirm_dialog.h"
#include "ui/main_menu.h"
#include "visual_pipeline/prepared_level.h"
#include "visual_pipeline/visual_preparation_pipeline.h"
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
  void LoadDeveloperConfigAtStartup();
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
  void HandleMapPreparingInput(const InputState& input);
  void HandleGameInput(const InputState& input);
  void UpdateMapPreparation();
  void UpdateGameView(const InputState& input);
  void Log3DMovementEvents(const InputState& input);
  void DrawMapPreparingScreen() const;
  void DrawGameOverlay() const;
  void UnloadFinalRenderTexture();
  bool LoadFinalRenderTexture();
  const Texture2D* FinalRenderTexture() const;
  void ActivateMenuItem(const MenuItem& item);
  bool StartNewGameFromConfig();
  bool ValidateMapPackagePath(const ProjectConfig& project_config);
  void OpenExitDialog();
  void RenderFrame();
  void SetMouseCapture(bool enabled);

  AppConfig config_;
  Logger logger_;
  WindowState window_state_;
  AppScreen screen_ = AppScreen::kMainMenu;
  bool exit_requested_ = false;
  bool window_initialized_ = false;
  InputSystem input_system_;
  Renderer renderer_;
  LevelRenderer level_renderer_;
  render3d::Level3DRenderer level_3d_renderer_;
  MenuRenderer menu_renderer_;
  DebugOverlay debug_overlay_;
  MainMenu main_menu_;
  GameSession game_session_;
  LevelLoader level_loader_;
  UiFont ui_font_;
  std::optional<ProjectConfig> project_config_;
  DeveloperConfig developer_config_;
  std::optional<LevelPackageSummary> loaded_level_summary_;
  std::optional<LevelData> loaded_level_;
  std::optional<visual_pipeline::PreparedLevel> prepared_level_;
  visual_pipeline::VisualPreparationPipeline visual_pipeline_;
  LevelViewState level_view_;
  render3d::Level3DViewState level_3d_view_;
  LevelRenderMode level_render_mode_ = LevelRenderMode::kRawTerrain;
  Texture2D final_render_texture_{};
  bool final_render_texture_loaded_ = false;
  bool final_render_texture_from_cpp_package_ = false;
  bool mouse_capture_active_ = false;
  int last_logged_3d_tile_x_ = -1;
  int last_logged_3d_tile_y_ = -1;
  unsigned int last_logged_3d_block_sequence_ = 0;
  float accumulated_mouse_dx_since_tile_ = 0.0F;
  float accumulated_mouse_dy_since_tile_ = 0.0F;
  float accumulated_abs_mouse_dx_since_tile_ = 0.0F;
  float accumulated_abs_mouse_dy_since_tile_ = 0.0F;
  float accumulated_mouse_wheel_since_tile_ = 0.0F;
  int mouse_sample_count_since_tile_ = 0;
  double last_preparation_step_time_ = -1.0;
  ServiceInfoOverlayData service_info_data_;
  double last_service_info_update_time_ = -1.0;
  std::optional<ConfirmDialog> confirm_dialog_;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_APP_APPLICATION_H_
