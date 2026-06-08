#ifndef SHOOT_AND_RUN_CPP_SRC_APP_APPLICATION_H_
#define SHOOT_AND_RUN_CPP_SRC_APP_APPLICATION_H_

/**
 * @file src/app/application.h
 * @brief Application configuration, lifecycle, startup, and runtime orchestration. Contains
 * public declarations for application.h.
 */

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
#include "render3d/model_registry.h"
#include "ui/confirm_dialog.h"
#include "ui/main_menu.h"
#include "visual_pipeline/prepared_level.h"
#include "visual_pipeline/visual_preparation_pipeline.h"
#include "window/window_state.h"

namespace sar {

/**
 * @brief Owns the application behavior and its runtime state.
 */
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
  /**
   * @brief Loads the project configuration and applies startup-level settings.
   */
  void LoadProjectConfigAtStartup();
  /**
   * @brief Loads optional developer diagnostics configuration.
   */
  void LoadDeveloperConfigAtStartup();
  /**
   * @brief Loads 3D asset registry metadata during application startup.
   */
  void LoadRender3DAssetRegistryAtStartup();
  /**
   * @brief Creates and configures the raylib window from resolved window settings.
   */
  void InitializeWindow();
  /**
   * @brief Loads the configured UI font and keeps the default font as a fallback.
   */
  void LoadUiFont();
  /**
   * @brief Shuts down owned runtime resources in a controlled order.
   */
  void ShutdownWindow();
  /**
   * @brief Updates window state from raylib for the current frame.
   */
  void UpdateWindowStateFromRaylib();
  /**
   * @brief Writes diagnostics for startup.
   */
  void LogStartup();
  /**
   * @brief Applies raylib log level.
   */
  void ApplyRaylibLogLevel();
  /**
   * @brief Refreshes throttled service overlay data such as memory usage.
   */
  void UpdateServiceInfo();
  /**
   * @brief Applies configured target FPS and frame pacing settings.
   */
  void ApplyFramePacing();
  /**
   * @brief Handles input.
   *
   * @param input Frame input snapshot for the current update.
   */
  void HandleInput(const InputState& input);
  /**
   * @brief Handles dialog input.
   *
   * @param input Frame input snapshot for the current update.
   */
  void HandleDialogInput(const InputState& input);
  /**
   * @brief Handles menu input.
   *
   * @param input Frame input snapshot for the current update.
   */
  void HandleMenuInput(const InputState& input);
  /**
   * @brief Handles map preparing input.
   *
   * @param input Frame input snapshot for the current update.
   */
  void HandleMapPreparingInput(const InputState& input);
  /**
   * @brief Handles game input.
   *
   * @param input Frame input snapshot for the current update.
   */
  void HandleGameInput(const InputState& input);
  /**
   * @brief Advances the visual map preparation pipeline when the preparation screen is active.
   */
  void UpdateMapPreparation();
  /**
   * @brief Updates game view for the current frame.
   *
   * @param input Frame input snapshot for the current update.
   */
  void UpdateGameView(const InputState& input);
  /**
   * @brief Writes throttled 3D movement events without logging every frame.
   *
   * @param input Frame input snapshot for the current update.
   */
  void Log3DMovementEvents(const InputState& input);
  /**
   * @brief Writes lifecycle events for the 3D camera intro sequence.
   */
  void Log3DCameraIntroEvents();
  /**
   * @brief Draws the map-preparation progress screen.
   */
  void DrawMapPreparingScreen() const;
  /**
   * @brief Draws service and debug overlays above the active game view.
   */
  void DrawGameOverlay() const;
  /**
   * @brief Draws the compact 3D player health HUD near the FPS counter.
   */
  void Draw3DPlayerHud() const;
  /**
   * @brief Releases the prepared final render texture when it is no longer needed.
   */
  void UnloadFinalRenderTexture();
  /**
   * @brief Loads the final prepared preview texture used by the game view.
   *
   * @return Result produced by the operation, when applicable.
   */
  bool LoadFinalRenderTexture();
  /**
   * @brief Returns the loaded final render texture when a prepared preview is available.
   *
   * @return Result produced by the operation, when applicable.
   */
  const Texture2D* FinalRenderTexture() const;
  /**
   * @brief Activates a main-menu item and runs the associated application action.
   *
   * @param item Menu item selected by the user.
   */
  void ActivateMenuItem(const MenuItem& item);
  /**
   * @brief Starts a new game session using the loaded project configuration and map package.
   *
   * @return Result produced by the operation, when applicable.
   */
  bool StartNewGameFromConfig();
  /**
   * @brief Validates map package path and reports failures.
   *
   * @param project_config Resolved configuration values used by the operation.
   * @return Result produced by the operation, when applicable.
   */
  bool ValidateMapPackagePath(const ProjectConfig& project_config);
  /**
   * @brief Opens the exit confirmation dialog with the safe option selected.
   */
  void OpenExitDialog();
  /**
   * @brief Renders one application frame for the active screen.
   */
  void RenderFrame();
  /**
   * @brief Enables or disables mouse capture for 3D gameplay input.
   *
   * @param enabled true to enable the behavior, false to disable it.
   */
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
  render3d::ModelRegistry3D model_registry_3d_;
  LevelRenderMode level_render_mode_ = LevelRenderMode::kRawTerrain;
  Texture2D final_render_texture_{};
  bool final_render_texture_loaded_ = false;
  bool final_render_texture_from_cpp_package_ = false;
  bool mouse_capture_active_ = false;
  int last_logged_3d_tile_x_ = -1;
  int last_logged_3d_tile_y_ = -1;
  unsigned int last_logged_3d_block_sequence_ = 0;
  unsigned int last_logged_3d_jump_sequence_ = 0;
  unsigned int last_logged_3d_transition_sequence_ = 0;
  unsigned int last_logged_3d_fall_sequence_ = 0;
  unsigned int last_logged_3d_health_sequence_ = 0;
  unsigned int last_logged_3d_intro_sequence_ = 0;
  float accumulated_mouse_dx_since_tile_ = 0.0F;
  float accumulated_mouse_dy_since_tile_ = 0.0F;
  float accumulated_abs_mouse_dx_since_tile_ = 0.0F;
  float accumulated_abs_mouse_dy_since_tile_ = 0.0F;
  float accumulated_mouse_wheel_since_tile_ = 0.0F;
  int mouse_sample_count_since_tile_ = 0;
  double last_3d_tile_log_time_ = -1000.0;
  double last_3d_block_log_time_ = -1000.0;
  double last_preparation_step_time_ = -1.0;
  ServiceInfoOverlayData service_info_data_;
  double last_service_info_update_time_ = -1.0;
  std::optional<ConfirmDialog> confirm_dialog_;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_APP_APPLICATION_H_
