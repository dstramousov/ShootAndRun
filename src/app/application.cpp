#include "app/application.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <sstream>
#include <string>
#include <utility>

#include "logging/thread_context.h"
#include "platform/terminal.h"
#include "ui/ui_layout.h"
#include "window/window_layout.h"

namespace sar {
namespace {

MonitorInfo CurrentMonitorInfo() {
  const int monitor = GetCurrentMonitor();
  const Vector2 position = GetMonitorPosition(monitor);
  return MonitorInfo{static_cast<int>(position.x), static_cast<int>(position.y),
                     GetMonitorWidth(monitor), GetMonitorHeight(monitor)};
}


int ScaledFontSize(const UiFont& font, const WindowState& window,
                   float multiplier) {
  const float size = static_cast<float>(font.base_size()) * multiplier *
                     window.ui_scale;
  return std::max(1, static_cast<int>(std::lround(size)));
}

std::string WindowStateToString(const WindowState& state) {
  std::ostringstream stream;
  stream << "monitor=" << state.monitor_width << 'x' << state.monitor_height
         << " window=" << state.width << 'x' << state.height << " pos="
         << state.x << ',' << state.y << " ui_scale=" << state.ui_scale;
  return stream.str();
}

}  // namespace

Application::Application(AppConfig config)
    : config_(std::move(config)),
      logger_(LoggerConfig{config_.log_level,
                           ShouldUseTerminalColor(config_.color_log)}) {}

int Application::Run() {
  SetCurrentThreadName("main");
  logger_.Info("app", "starting application");

  LoadProjectConfigAtStartup();
  InitializeWindow();
  LogStartup();

  while (!exit_requested_) {
    if (WindowShouldClose() && !confirm_dialog_.has_value()) {
      OpenExitDialog();
    }

    UpdateWindowStateFromRaylib();
    HandleInput(input_system_.Poll());
    RenderFrame();
  }

  ShutdownWindow();
  logger_.Info("app", "application stopped");
  return 0;
}

void Application::LoadProjectConfigAtStartup() {
  const ProjectConfigResult result =
      LoadProjectConfig(config_.project_config_path);
  if (!result.ok) {
    logger_.Error("config", result.error);
    return;
  }

  project_config_ = result.config;
  config_.window = project_config_->window_config;
  logger_.Info("config", project_config_->Dump());
}

void Application::InitializeWindow() {
  if (config_.window.resizable) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  }

  SetExitKey(KEY_NULL);
  InitWindow(config_.window.preferred_width, config_.window.preferred_height,
             config_.app_name.c_str());
  window_initialized_ = true;

  window_state_ = CalculateWindowState(CurrentMonitorInfo(), config_.window);
  SetWindowSize(window_state_.width, window_state_.height);
  SetWindowPosition(window_state_.x, window_state_.y);
  ApplyFramePacing();
  LoadUiFont();
}

void Application::LoadUiFont() {
  if (!project_config_.has_value()) {
    logger_.Warn("font", "project config is not loaded; using default font");
    return;
  }

  std::string error;
  if (!ui_font_.Load(project_config_->ui_font_path,
                     project_config_->ui_font_size, &error)) {
    logger_.Warn("font", error + "; using default raylib font");
    return;
  }

  logger_.Info("font", "loaded path=" + project_config_->ui_font_path.string() +
                           " size=" +
                           std::to_string(project_config_->ui_font_size));
}

void Application::ShutdownWindow() {
  if (window_initialized_) {
    ui_font_.Reset();
    CloseWindow();
    window_initialized_ = false;
  }
}

void Application::UpdateWindowStateFromRaylib() {
  window_state_.width = GetScreenWidth();
  window_state_.height = GetScreenHeight();
  window_state_.ui_scale = CalculateUiScale(window_state_.width,
                                            window_state_.height,
                                            config_.window);
}

void Application::LogStartup() {
  logger_.Info("app", "started version=" + config_.version);
  logger_.Info("log", std::string("level=") +
                          std::string(LogLevelName(config_.log_level)));
  logger_.Info("window", WindowStateToString(window_state_));
  logger_.Debug("menu", main_menu_.Dump());
}

void Application::ApplyFramePacing() {
  SetTargetFPS(config_.target_fps);
}

void Application::HandleInput(const InputState& input) {
  if (confirm_dialog_.has_value()) {
    HandleDialogInput(input);
    return;
  }

  switch (screen_) {
    case AppScreen::kMainMenu:
    case AppScreen::kSettings:
      HandleMenuInput(input);
      break;
    case AppScreen::kGame:
      HandleGameInput(input);
      break;
  }
}

void Application::HandleDialogInput(const InputState& input) {
  ConfirmDialog& dialog = *confirm_dialog_;

  if (input.left_pressed || input.right_pressed) {
    dialog.Toggle();
    logger_.Debug("dialog", dialog.Dump());
  }

  const ConfirmDialogLayout layout = CalculateConfirmDialogLayout(window_state_);
  if (input.left_mouse_pressed) {
    if (ContainsPoint(layout.yes_bounds, input.mouse_position)) {
      dialog.SelectYes();
    } else if (ContainsPoint(layout.no_bounds, input.mouse_position)) {
      dialog.SelectNo();
    }
  }

  if (input.cancel_pressed) {
    logger_.Info("dialog", "confirmation cancelled");
    confirm_dialog_.reset();
    return;
  }

  if (input.confirm_pressed || input.left_mouse_pressed) {
    if (dialog.selected_choice() == DialogChoice::kYes) {
      logger_.Info("app", "exit confirmed by user");
      exit_requested_ = true;
    } else {
      logger_.Info("app", "exit cancelled by user");
      confirm_dialog_.reset();
    }
  }
}

void Application::HandleMenuInput(const InputState& input) {
  if (input.cancel_pressed) {
    OpenExitDialog();
    return;
  }

  if (input.up_pressed) {
    main_menu_.SelectPrevious();
    logger_.Debug("menu", main_menu_.Dump());
  }

  if (input.down_pressed) {
    main_menu_.SelectNext();
    logger_.Debug("menu", main_menu_.Dump());
  }

  const auto layouts = CalculateMainMenuLayout(
      static_cast<int>(main_menu_.items().size()), window_state_);
  const int hovered_index = HitTestMenuItem(layouts, input.mouse_position);
  if (hovered_index >= 0 && main_menu_.SelectIndex(hovered_index)) {
    logger_.Debug("menu", main_menu_.Dump());
  }

  if (input.confirm_pressed || input.left_mouse_pressed) {
    const MenuItem* item = main_menu_.ActivateSelected();
    if (item == nullptr) {
      logger_.Debug("menu", "disabled item activation ignored");
      return;
    }

    if (input.left_mouse_pressed && hovered_index < 0) {
      return;
    }

    ActivateMenuItem(*item);
  }
}

void Application::HandleGameInput(const InputState& input) {
  if (input.cancel_pressed || input.cancel_down) {
    screen_ = AppScreen::kMainMenu;
    logger_.Info("game", "returned to main menu");
  }
}

void Application::ActivateMenuItem(const MenuItem& item) {
  logger_.Info("menu", "item activated id=" + item.id);

  switch (item.action) {
    case MenuAction::kNewGame:
      if (!StartNewGameFromConfig()) {
        return;
      }
      break;
    case MenuAction::kLoadGame:
      logger_.Warn("saves", "load game is not implemented");
      break;
    case MenuAction::kSaveGame:
      logger_.Warn("saves", "save game is not available from main menu");
      break;
    case MenuAction::kSettings:
      screen_ = AppScreen::kSettings;
      logger_.Info("settings", "settings screen is not implemented");
      screen_ = AppScreen::kMainMenu;
      break;
    case MenuAction::kExit:
      OpenExitDialog();
      break;
  }
}

bool Application::StartNewGameFromConfig() {
  if (!project_config_.has_value()) {
    logger_.Error("config", "project config is not loaded");
    return false;
  }

  if (!ValidateMapPackagePath(*project_config_)) {
    return false;
  }

  const LevelLoadResult level_result = level_loader_.LoadBasicPackage(
      project_config_->map_package_path);
  if (!level_result.ok) {
    logger_.Error("level", level_result.error);
    return false;
  }

  loaded_level_summary_ = level_result.summary;
  logger_.Info("level", loaded_level_summary_->Dump());

  game_session_.StartNewGame();
  screen_ = AppScreen::kGame;
  ApplyFramePacing();
  logger_.Info("game", "new game session started");
  return true;
}

bool Application::ValidateMapPackagePath(
    const ProjectConfig& project_config) {
  std::error_code error_code;
  const bool exists = std::filesystem::exists(
      project_config.map_package_path, error_code);
  if (error_code) {
    logger_.Error("config", "failed to inspect map package path=" +
                                project_config.map_package_path.string() +
                                " reason=" + error_code.message());
    return false;
  }

  if (!exists) {
    logger_.Error("config", "map package path does not exist: " +
                                project_config.map_package_path.string());
    return false;
  }

  const bool is_directory = std::filesystem::is_directory(
      project_config.map_package_path, error_code);
  if (error_code) {
    logger_.Error("config", "failed to check map package directory=" +
                                project_config.map_package_path.string() +
                                " reason=" + error_code.message());
    return false;
  }

  if (!is_directory) {
    logger_.Error("config", "map package path is not a directory: " +
                                project_config.map_package_path.string());
    return false;
  }

  logger_.Info("game", "map package selected path=" +
                           project_config.map_package_path.string());
  return true;
}

void Application::OpenExitDialog() {
  confirm_dialog_.emplace("Exit game", "Unsaved progress may be lost.");
  logger_.Debug("dialog", "opened type=exit_confirmation default=no");
}

void Application::RenderFrame() {
  renderer_.BeginFrame();
  renderer_.DrawTitle(config_.app_name, window_state_, ui_font_);

  if (screen_ == AppScreen::kMainMenu || screen_ == AppScreen::kSettings) {
    menu_renderer_.DrawMainMenu(main_menu_, window_state_, ui_font_);
  } else if (screen_ == AppScreen::kGame) {
    const int title_size = ScaledFontSize(ui_font_, window_state_, 1.0F);
    const int hint_size = ScaledFontSize(ui_font_, window_state_, 0.8F);
    const int summary_size = ScaledFontSize(ui_font_, window_state_, 0.65F);
    ui_font_.DrawTextLine("Game screen placeholder", 48, 120, title_size,
                          Color{230, 230, 240, 255});
    ui_font_.DrawTextLine("Press Esc to return to main menu.", 48, 156,
                          hint_size, Color{170, 175, 195, 255});
    if (loaded_level_summary_.has_value()) {
      ui_font_.DrawTextLine(loaded_level_summary_->Dump(), 48, 196,
                            summary_size, Color{145, 150, 170, 255});
    }
  }

  if (confirm_dialog_.has_value()) {
    menu_renderer_.DrawConfirmDialog(*confirm_dialog_, window_state_,
                                     ui_font_);
  }

  renderer_.DrawFps(window_state_, ui_font_);

  if (config_.debug_overlay_enabled) {
    debug_overlay_.Draw(config_.version, screen_, window_state_, ui_font_);
  }

  renderer_.EndFrame();
}

}  // namespace sar
