#include "app/application.h"

#include <raylib.h>

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

void Application::InitializeWindow() {
  if (config_.window.resizable) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  }

  SetExitKey(KEY_NULL);
  InitWindow(config_.window.base_width, config_.window.base_height,
             config_.app_name.c_str());
  window_initialized_ = true;

  window_state_ = CalculateWindowState(CurrentMonitorInfo(), config_.window);
  SetWindowSize(window_state_.width, window_state_.height);
  SetWindowPosition(window_state_.x, window_state_.y);
  ApplyFramePacing();
}

void Application::ShutdownWindow() {
  if (window_initialized_) {
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
      game_session_.StartNewGame();
      screen_ = AppScreen::kGame;
      ApplyFramePacing();
      logger_.Info("game", "new game session started");
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

void Application::OpenExitDialog() {
  confirm_dialog_.emplace("Exit game", "Unsaved progress may be lost.");
  logger_.Debug("dialog", "opened type=exit_confirmation default=no");
}

void Application::RenderFrame() {
  renderer_.BeginFrame();
  renderer_.DrawTitle(config_.app_name, window_state_);

  if (screen_ == AppScreen::kMainMenu || screen_ == AppScreen::kSettings) {
    menu_renderer_.DrawMainMenu(main_menu_, window_state_);
  } else if (screen_ == AppScreen::kGame) {
    DrawText("Game screen placeholder", 48, 120, 24, Color{230, 230, 240, 255});
    DrawText("Press Esc to return to main menu.", 48, 156, 18,
             Color{170, 175, 195, 255});
  }

  if (confirm_dialog_.has_value()) {
    menu_renderer_.DrawConfirmDialog(*confirm_dialog_, window_state_);
  }

  renderer_.DrawFps(window_state_);

  if (config_.debug_overlay_enabled) {
    debug_overlay_.Draw(config_.version, screen_, window_state_);
  }

  renderer_.EndFrame();
}

}  // namespace sar
