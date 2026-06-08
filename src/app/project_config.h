#ifndef SHOOT_AND_RUN_CPP_SRC_APP_PROJECT_CONFIG_H_
#define SHOOT_AND_RUN_CPP_SRC_APP_PROJECT_CONFIG_H_

#include <filesystem>
#include <string>

#include "window/window_config.h"
#include "visual_pipeline/visual_pipeline_config.h"

namespace sar {

enum class RaylibLogLevel {
  kTrace,
  kDebug,
  kInfo,
  kWarning,
  kError,
  kFatal,
  kNone,
};

/**
 * @brief Returns the configuration name of a raylib log level.
 *
 * @param level Raylib trace log level.
 * @return Stable lowercase configuration name.
 */
const char* RaylibLogLevelName(RaylibLogLevel level);

struct ServiceInfoConfig {
  bool enabled = true;
  bool show_memory = true;
  int update_interval_ms = 1000;
};

struct Player3DLogConfig {
  bool enabled = true;
  bool include_mouse = true;
  int tile_log_min_interval_ms = 250;
  int blocked_log_min_interval_ms = 600;
};

struct Render3DPerfConfig {
  int visible_radius_tiles = 48;
  int culling_deadzone_tiles = 4;
};

struct Player3DMovementConfig {
  float move_speed_tiles_per_sec = 4.25F;
  float acceleration_tiles_per_sec2 = 28.0F;
  float deceleration_tiles_per_sec2 = 34.0F;
  float mouse_turn_sensitivity_rad = 0.0031F;
  float movement_multiplier_smooth_speed = 14.0F;
  float jump_duration_sec = 0.30F;
  float jump_arc_elevation_units = 0.62F;
  float jump_horizontal_speed_multiplier = 1.05F;
  float jump_air_control_multiplier = 0.82F;
  float jump_min_running_speed_tiles_per_sec = 1.00F;
};

struct ProjectConfig {
  std::filesystem::path map_package_path;
  std::filesystem::path ui_font_path = "data/fonts/PressStart2P-Regular.ttf";
  int ui_font_size = 24;
  RaylibLogLevel raylib_log_level = RaylibLogLevel::kWarning;
  WindowConfig window_config;
  ServiceInfoConfig service_info;
  Player3DLogConfig player3d_log;
  Render3DPerfConfig render3d_perf;
  Player3DMovementConfig player3d_movement;
  visual_pipeline::VisualPipelineConfig visual_pipeline_config;

  /**
   * @brief Returns a readable dump of the project configuration.
   *
   * @return String representation for logs and diagnostics.
   */
  std::string Dump() const;
};

struct ProjectConfigResult {
  bool ok = false;
  ProjectConfig config;
  std::string error;
};

/**
 * @brief Loads project configuration from a JSON file.
 *
 * The loader requires the `map_package_path` string field. Font, window,
 * service-info, raylib log and visual pipeline settings are optional and use safe defaults
 * when they are not present. Unknown fields are ignored so the format can be
 * extended later.
 *
 * @param config_path Path to the project configuration file.
 * @return Load result with either configuration data or an error message.
 */
ProjectConfigResult LoadProjectConfig(
    const std::filesystem::path& config_path);

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_APP_PROJECT_CONFIG_H_
