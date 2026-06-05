#include "app/application.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "logging/thread_context.h"
#include "platform/memory_info.h"
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

int ToRaylibTraceLogLevel(RaylibLogLevel level) {
  switch (level) {
    case RaylibLogLevel::kTrace:
      return LOG_TRACE;
    case RaylibLogLevel::kDebug:
      return LOG_DEBUG;
    case RaylibLogLevel::kInfo:
      return LOG_INFO;
    case RaylibLogLevel::kWarning:
      return LOG_WARNING;
    case RaylibLogLevel::kError:
      return LOG_ERROR;
    case RaylibLogLevel::kFatal:
      return LOG_FATAL;
    case RaylibLogLevel::kNone:
      return LOG_NONE;
  }

  return LOG_WARNING;
}


LogHighlightScope ParseHighlightScope(const std::string& value) {
  if (value == "full_line") {
    return LogHighlightScope::kFullLine;
  }
  return LogHighlightScope::kMessage;
}

std::vector<LogHighlightRule> BuildLoggerHighlightRules(
    const DeveloperLogConfig& config) {
  std::vector<LogHighlightRule> rules;
  rules.reserve(config.highlight_rules.size());
  for (const DeveloperHighlightRuleConfig& source : config.highlight_rules) {
    LogHighlightRule rule;
    rule.name = source.name;
    rule.regex_pattern = source.regex_pattern;
    rule.color = source.color;
    rule.scope = ParseHighlightScope(source.scope);
    rule.case_sensitive = source.case_sensitive;
    rules.push_back(std::move(rule));
  }
  return rules;
}

LoggerConfig BuildInitialLoggerConfig(const AppConfig& config) {
  LoggerConfig logger_config;
  logger_config.min_level = config.log_level;
  logger_config.color_enabled = ShouldUseTerminalColor(config.color_log);
  logger_config.show_execution_context = true;
  logger_config.highlight_rules = {};
  return logger_config;
}


std::string FormatDouble(double value, int precision) {
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(precision) << value;
  return stream.str();
}

std::string PipelineStepLabel(
    const visual_pipeline::PipelineStepReport& report) {
  return "step " + std::to_string(report.step_index) + "/" +
         std::to_string(report.total_steps) + " name=\"" +
         report.step_name + "\"";
}


std::string TileShareLine(std::string_view label, int count, int total_tiles) {
  const double percent = total_tiles > 0
                             ? static_cast<double>(count) * 100.0 /
                                   static_cast<double>(total_tiles)
                             : 0.0;
  std::ostringstream stream;
  stream << "  " << std::left << std::setw(14) << label << ": "
         << std::right << std::setw(7) << count << " tiles = "
         << std::setw(5) << FormatDouble(percent, 1) << "%";
  return stream.str();
}

std::string CountLine(std::string_view label, int count) {
  std::ostringstream stream;
  stream << "  " << std::left << std::setw(14) << label << ": "
         << std::right << std::setw(7) << count;
  return stream.str();
}

std::string BuildMapPreparationReport(
    const LevelData& level,
    const visual_pipeline::PreparedLevel& prepared_level,
    const std::filesystem::path& map_package_path) {
  const int total_tiles = level.size.width * level.size.height;
  std::ostringstream report;
  report << "Map preparation report:\n";
  report << "  output: " << map_package_path.string() << "\n";
  report << "  map:    " << level.size.width << " x "
         << level.size.height << " = " << total_tiles << " tiles\n";
  report << "  tile:   " << level.size.tile_size << " px\n";
  report << "  source: "
         << visual_pipeline::PreparedLevelSourceName(prepared_level.source)
         << "\n";
  report << "  status: " << (prepared_level.ready ? "ok" : "not ready")
         << "\n\n";

  if (prepared_level.prepared_visual_map.loaded) {
    const visual_pipeline::VisualMapData& visual_map =
        prepared_level.prepared_visual_map;
    report << "Prepared visual map:\n";
    report << "  profile: " << visual_map.visual_profile_id << "\n";
    report << CountLine("layers", visual_map.visual_layer_count) << "\n";
    report << CountLine("unique tiles", visual_map.unique_tile_id_count)
           << "\n";
    report << CountLine("objects", visual_map.visual_object_count) << "\n";
    report << CountLine("chunks", visual_map.visual_chunk_count) << "\n";
    report << "  contract: gameplay="
           << (visual_map.changes_gameplay ? "changed" : "unchanged")
           << " markers="
           << (visual_map.moves_markers ? "moved" : "unchanged")
           << " collision="
           << (visual_map.changes_collision ? "changed" : "unchanged")
           << "\n\n";
  }

  if (prepared_level.semantic_masks.IsValid()) {
    const visual_pipeline::SemanticMaskSummary& summary =
        prepared_level.semantic_masks.summary;
    report << "Terrain:\n";
    report << TileShareLine("forest", summary.forest_tiles, total_tiles)
           << "\n";
    report << TileShareLine("road", summary.road_tiles, total_tiles) << "\n";
    report << TileShareLine("water", summary.water_tiles, total_tiles)
           << "\n";
    report << TileShareLine("swamp", summary.swamp_tiles, total_tiles)
           << "\n";
    report << TileShareLine("ruins", summary.ruins_tiles, total_tiles)
           << "\n";
    report << TileShareLine("wall", summary.wall_tiles, total_tiles) << "\n";
    report << TileShareLine("open ground", summary.open_ground_tiles,
                            total_tiles)
           << "\n";
    if (summary.unknown_tiles > 0) {
      report << TileShareLine("unknown", summary.unknown_tiles, total_tiles)
             << "\n";
    }

    report << "\nRuntime:\n";
    report << TileShareLine("walkable", summary.walkable_tiles, total_tiles)
           << "\n";
    report << TileShareLine("blocked", summary.blocked_tiles, total_tiles)
           << "\n";
    report << TileShareLine("vision block", summary.vision_blocked_tiles,
                            total_tiles)
           << "\n";
    report << CountLine("cover", summary.cover_tiles) << "\n";
    report << CountLine("concealment", summary.concealment_tiles) << "\n";
    report << CountLine("low ground", summary.low_ground_tiles) << "\n";
    report << CountLine("elevated", summary.elevated_tiles) << "\n\n";
  }

  if (prepared_level.terrain_regions.IsValid()) {
    const visual_pipeline::TerrainRegionSummary& summary =
        prepared_level.terrain_regions.summary;
    report << "Regions:\n";
    report << CountLine("total", summary.total_regions) << "\n";
    report << CountLine("forest", summary.forest_regions) << "\n";
    report << CountLine("open", summary.open_ground_regions) << "\n";
    report << CountLine("road", summary.road_regions) << "\n";
    report << CountLine("water", summary.water_regions) << "\n";
    report << CountLine("wall", summary.wall_regions) << "\n";
    report << CountLine("tiny", summary.tiny_regions) << "\n";
    report << CountLine("largest", summary.largest_region_area) << "\n\n";
  }

  if (prepared_level.region_borders.IsValid()) {
    const visual_pipeline::RegionBorderSummary& summary =
        prepared_level.region_borders.summary;
    report << "Borders:\n";
    report << CountLine("tiles", summary.border_tile_count) << "\n";
    report << CountLine("edge", summary.edge_tile_count) << "\n";
    report << CountLine("corner", summary.corner_tile_count) << "\n";
    report << CountLine("complex", summary.complex_tile_count) << "\n";
    report << CountLine("map edge", summary.map_edge_tile_count) << "\n";
  }

  std::vector<std::string> warnings;
  if (prepared_level.semantic_masks.IsValid() &&
      prepared_level.semantic_masks.summary.unknown_tiles > 0) {
    warnings.push_back("unknown terrain tiles=" +
                       std::to_string(
                           prepared_level.semantic_masks.summary.unknown_tiles));
  }
  if (prepared_level.terrain_regions.IsValid() &&
      prepared_level.terrain_regions.summary.tiny_regions > 0) {
    warnings.push_back("tiny terrain regions=" +
                       std::to_string(
                           prepared_level.terrain_regions.summary.tiny_regions));
  }
  if (prepared_level.region_borders.IsValid() &&
      prepared_level.region_borders.summary.complex_tile_count > 0) {
    warnings.push_back("complex border tiles=" +
                       std::to_string(prepared_level.region_borders.summary
                                          .complex_tile_count));
  }
  for (const std::string& warning :
       prepared_level.prepared_visual_map.warnings) {
    warnings.push_back(warning);
  }

  if (!warnings.empty()) {
    report << "\nWarnings:\n";
    for (const std::string& warning : warnings) {
      report << "  - " << warning << "\n";
    }
  }

  return report.str();
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
      logger_(BuildInitialLoggerConfig(config_)) {}

int Application::Run() {
  SetCurrentThreadName("main");
  LoadDeveloperConfigAtStartup();
  LoadProjectConfigAtStartup();
  logger_.Info("app", "starting application");
  InitializeWindow();
  LogStartup();

  while (!exit_requested_) {
    if (WindowShouldClose() && !confirm_dialog_.has_value()) {
      OpenExitDialog();
    }

    UpdateWindowStateFromRaylib();
    HandleInput(input_system_.Poll());
    UpdateMapPreparation();
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

void Application::LoadDeveloperConfigAtStartup() {
  const DeveloperConfigResult result =
      LoadDeveloperConfig(config_.developer_config_path);
  if (!result.ok) {
    logger_.Warn("developer_config", result.error);
    return;
  }

  developer_config_ = result.config;
  logger_.set_show_execution_context(
      developer_config_.log.show_execution_context);
  logger_.set_color_enabled(ShouldUseTerminalColor(config_.color_log) &&
                            developer_config_.log.color_enabled);
  logger_.set_highlight_rules(
      BuildLoggerHighlightRules(developer_config_.log));

  if (!result.found) {
    logger_.Debug("developer_config",
                  "developer log config not found path=" +
                      config_.developer_config_path.string() +
                      "; using defaults");
    return;
  }

  if (developer_config_.log.enabled) {
    logger_.Info("developer_config", developer_config_.Dump());
  }
}

void Application::InitializeWindow() {
  ApplyRaylibLogLevel();

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

void Application::ApplyRaylibLogLevel() {
  const RaylibLogLevel level = project_config_.has_value()
                                   ? project_config_->raylib_log_level
                                   : RaylibLogLevel::kWarning;
  SetTraceLogLevel(ToRaylibTraceLogLevel(level));
  logger_.Debug("raylib", std::string("trace log level=") +
                              RaylibLogLevelName(level));
}

void Application::UpdateServiceInfo() {
  if (!project_config_.has_value() ||
      !project_config_->service_info.enabled) {
    service_info_data_ = {};
    return;
  }

  service_info_data_.show_memory = project_config_->service_info.show_memory;
  if (!project_config_->service_info.show_memory) {
    service_info_data_.memory_available = false;
    service_info_data_.resident_memory_bytes = 0;
    return;
  }

  const double now = GetTime();
  const double interval_seconds =
      static_cast<double>(project_config_->service_info.update_interval_ms) /
      1000.0;
  if (last_service_info_update_time_ >= 0.0 &&
      now - last_service_info_update_time_ < interval_seconds) {
    return;
  }

  last_service_info_update_time_ = now;
  const ProcessMemoryInfo memory = ReadCurrentProcessMemoryInfo();
  service_info_data_.memory_available = memory.available;
  service_info_data_.resident_memory_bytes = memory.resident_bytes;
  if (!memory.available) {
    logger_.Debug("service_info", memory.error);
  }
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
    case AppScreen::kMapPreparing:
      HandleMapPreparingInput(input);
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

void Application::HandleMapPreparingInput(const InputState& input) {
  if (input.cancel_pressed) {
    visual_pipeline_ = visual_pipeline::VisualPreparationPipeline();
    prepared_level_.reset();
    screen_ = AppScreen::kMainMenu;
    logger_.Info("visual_pipeline", "map preparation cancelled");
  }
}

void Application::HandleGameInput(const InputState& input) {
  if (input.cancel_pressed || input.cancel_down) {
    screen_ = AppScreen::kMainMenu;
    logger_.Info("game", "returned to main menu");
    return;
  }

  UpdateGameCamera(input);
}

void Application::UpdateMapPreparation() {
  if (screen_ != AppScreen::kMapPreparing || !loaded_level_.has_value()) {
    return;
  }

  constexpr double kStepIntervalSeconds = 0.12;
  const double now = GetTime();
  if (last_preparation_step_time_ >= 0.0 &&
      now - last_preparation_step_time_ < kStepIntervalSeconds) {
    return;
  }

  last_preparation_step_time_ = now;
  const visual_pipeline::PipelineProgress& before =
      visual_pipeline_.progress();
  if (developer_config_.log.visual_pipeline_diagnostics &&
      developer_config_.log.visual_pipeline_step_details) {
    logger_.Debug("visual_pipeline",
                  "step " + std::to_string(before.completed_steps + 1) +
                      "/" + std::to_string(before.total_steps) +
                      " started name=\"" + before.current_step_name + "\"");
  }

  visual_pipeline_.AdvanceOneStep(*loaded_level_);
  const visual_pipeline::PipelineStepReport& report =
      visual_pipeline_.last_step_report();

  if (report.step_index > 0 &&
      developer_config_.log.visual_pipeline_diagnostics &&
      developer_config_.log.visual_pipeline_step_details) {
    const std::string status = report.success ? "done" : "failed";
    logger_.Debug("visual_pipeline",
                  PipelineStepLabel(report) + " " + status +
                      " duration_ms=" + FormatDouble(report.duration_ms, 2));
    for (const std::string& summary : report.summaries) {
      logger_.Debug("map_analysis", summary);
    }
    for (const std::string& warning : report.warnings) {
      logger_.Debug("map_analysis", "warning: " + warning);
    }
  }

  if (visual_pipeline_.progress().failed) {
    logger_.Error("visual_pipeline", visual_pipeline_.progress().error);
    screen_ = AppScreen::kMainMenu;
    return;
  }

  if (!visual_pipeline_.finished()) {
    return;
  }

  prepared_level_ = visual_pipeline_.prepared_level();
  if (developer_config_.log.visual_pipeline_diagnostics &&
      developer_config_.log.visual_pipeline_summary &&
      project_config_.has_value()) {
    logger_.Info("map_analysis",
                 BuildMapPreparationReport(*loaded_level_, *prepared_level_,
                                           project_config_->map_package_path));
  }
  logger_.Debug("visual_pipeline", prepared_level_->Dump());
  InitializeLevelView(*loaded_level_, &level_view_);
  ClampLevelViewToMap(*loaded_level_, window_state_, &level_view_);
  logger_.Info("camera", LevelViewStateToString(level_view_));

  game_session_.StartNewGame();
  screen_ = AppScreen::kGame;
  ApplyFramePacing();
  logger_.Info("game", "new game session started");
}

void Application::UpdateGameCamera(const InputState& input) {
  if (!loaded_level_.has_value()) {
    return;
  }

  float direction_x = 0.0F;
  float direction_y = 0.0F;
  if (input.left_down) {
    direction_x -= 1.0F;
  }
  if (input.right_down) {
    direction_x += 1.0F;
  }
  if (input.up_down) {
    direction_y -= 1.0F;
  }
  if (input.down_down) {
    direction_y += 1.0F;
  }

  if (direction_x != 0.0F || direction_y != 0.0F) {
    const float length = std::sqrt(direction_x * direction_x +
                                   direction_y * direction_y);
    direction_x /= length;
    direction_y /= length;

    const float dt = std::min(GetFrameTime(), 0.05F);
    const float pan_speed = level_view_.pan_speed_px_per_sec /
                            std::max(level_view_.zoom, 0.1F);
    level_view_.target_x += direction_x * pan_speed * dt;
    level_view_.target_y += direction_y * pan_speed * dt;
  }

  if (input.mouse_wheel_delta != 0.0F) {
    const float zoom_multiplier = std::pow(1.10F, input.mouse_wheel_delta);
    level_view_.zoom *= zoom_multiplier;
  }

  ClampLevelViewToMap(*loaded_level_, window_state_, &level_view_);
}

void Application::DrawMapPreparingScreen() const {
  const visual_pipeline::PipelineProgress& progress =
      visual_pipeline_.progress();
  const int title_size = ScaledFontSize(ui_font_, window_state_, 1.15F);
  const int text_size = ScaledFontSize(ui_font_, window_state_, 0.72F);
  const int bar_width = static_cast<int>(760.0F * window_state_.ui_scale);
  const int bar_height = static_cast<int>(26.0F * window_state_.ui_scale);
  const int center_x = window_state_.width / 2;
  const int start_x = center_x - bar_width / 2;
  const int start_y = window_state_.height / 2 -
                      static_cast<int>(78.0F * window_state_.ui_scale);

  const std::string title = "Preparing map";
  const int title_width = ui_font_.MeasureTextWidth(title, title_size);
  ui_font_.DrawTextLine(title, center_x - title_width / 2, start_y,
                        title_size, Color{235, 235, 245, 255});

  const int bar_y = start_y + static_cast<int>(58.0F * window_state_.ui_scale);
  const float normalized = progress.Normalized();
  const int filled_width = static_cast<int>(
      std::round(static_cast<float>(bar_width) * normalized));
  DrawRectangleRounded(Rectangle{static_cast<float>(start_x),
                                 static_cast<float>(bar_y),
                                 static_cast<float>(bar_width),
                                 static_cast<float>(bar_height)},
                       0.25F, 10, Color{36, 38, 50, 255});
  DrawRectangleRounded(Rectangle{static_cast<float>(start_x),
                                 static_cast<float>(bar_y),
                                 static_cast<float>(filled_width),
                                 static_cast<float>(bar_height)},
                       0.25F, 10, Color{120, 150, 105, 255});
  DrawRectangleRoundedLinesEx(Rectangle{static_cast<float>(start_x),
                                        static_cast<float>(bar_y),
                                        static_cast<float>(bar_width),
                                        static_cast<float>(bar_height)},
                              0.25F, 10, 1.5F,
                              Color{155, 165, 185, 255});

  const std::string step_text =
      "Step " + std::to_string(progress.completed_steps +
                               (progress.finished ? 0 : 1)) +
      " / " + std::to_string(progress.total_steps) + ": " +
      progress.current_step_name;
  const int step_width = ui_font_.MeasureTextWidth(step_text, text_size);
  ui_font_.DrawTextLine(step_text, center_x - step_width / 2,
                        bar_y + static_cast<int>(48.0F * window_state_.ui_scale),
                        text_size, Color{190, 198, 215, 255});

  const std::string hint = "Esc: cancel preparation";
  const int hint_width = ui_font_.MeasureTextWidth(hint, text_size);
  ui_font_.DrawTextLine(hint, center_x - hint_width / 2,
                        bar_y + static_cast<int>(86.0F * window_state_.ui_scale),
                        text_size, Color{130, 138, 155, 255});
}

void Application::DrawGameOverlay() const {
  if (!loaded_level_summary_.has_value()) {
    return;
  }

  const int font_size = ScaledFontSize(ui_font_, window_state_, 0.65F);
  const int x = static_cast<int>(24.0F * window_state_.ui_scale);
  int y = window_state_.height -
          static_cast<int>(72.0F * window_state_.ui_scale);
  const int line_step = font_size +
                        static_cast<int>(6.0F * window_state_.ui_scale);
  const Color color = Color{185, 190, 205, 255};

  ui_font_.DrawTextLine(loaded_level_summary_->Dump(), x, y, font_size,
                        color);
  y += line_step;
  ui_font_.DrawTextLine(LevelViewStateToString(level_view_), x, y,
                        font_size, color);
  if (prepared_level_.has_value()) {
    y += line_step;
    ui_font_.DrawTextLine(prepared_level_->Dump(), x, y, font_size, color);
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
  loaded_level_ = level_result.level;
  prepared_level_.reset();
  visual_pipeline::VisualPreparationOptions preparation_options;
  preparation_options.map_package_path = project_config_->map_package_path;
  preparation_options.visual_pipeline_config =
      project_config_->visual_pipeline_config;
  visual_pipeline_.Start(*loaded_level_, std::move(preparation_options));
  last_preparation_step_time_ = -1.0;
  screen_ = AppScreen::kMapPreparing;
  logger_.Info("level", loaded_level_summary_->Dump());
  if (developer_config_.log.visual_pipeline_diagnostics &&
      developer_config_.log.visual_pipeline_step_details) {
    logger_.Debug("visual_pipeline",
                  "started steps=" +
                      std::to_string(visual_pipeline_.progress().total_steps) +
                      " map=" + std::to_string(loaded_level_->size.width) +
                      "x" + std::to_string(loaded_level_->size.height) +
                      " tile_size=" +
                      std::to_string(loaded_level_->size.tile_size));
    logger_.Debug("visual_pipeline", visual_pipeline_.progress().Dump());
  }
  ApplyFramePacing();
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

  if (screen_ == AppScreen::kMainMenu || screen_ == AppScreen::kSettings) {
    renderer_.DrawTitle(config_.app_name, window_state_, ui_font_);
    menu_renderer_.DrawMainMenu(main_menu_, window_state_, ui_font_);
  } else if (screen_ == AppScreen::kMapPreparing) {
    DrawMapPreparingScreen();
  } else if (screen_ == AppScreen::kGame) {
    if (loaded_level_.has_value()) {
      ClampLevelViewToMap(*loaded_level_, window_state_, &level_view_);
      level_renderer_.DrawTerrain(*loaded_level_, level_view_, window_state_);
      DrawGameOverlay();
    } else {
      const int title_size = ScaledFontSize(ui_font_, window_state_, 1.0F);
      ui_font_.DrawTextLine("Game screen: no level loaded", 48, 120,
                            title_size, Color{230, 230, 240, 255});
    }
  }

  if (confirm_dialog_.has_value()) {
    menu_renderer_.DrawConfirmDialog(*confirm_dialog_, window_state_,
                                     ui_font_);
  }

  renderer_.DrawFps(window_state_, ui_font_);

  UpdateServiceInfo();
  if (config_.debug_overlay_enabled &&
      project_config_.has_value() &&
      project_config_->service_info.enabled) {
    debug_overlay_.Draw(config_.version, screen_, window_state_, ui_font_,
                        service_info_data_);
  }

  renderer_.EndFrame();
}

}  // namespace sar
