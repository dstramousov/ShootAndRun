#ifndef SHOOT_AND_RUN_CPP_SRC_APP_PROJECT_CONFIG_H_
#define SHOOT_AND_RUN_CPP_SRC_APP_PROJECT_CONFIG_H_

/**
 * @file src/app/project_config.h
 * @brief Application configuration, lifecycle, startup, and runtime orchestration. Contains
 * public declarations for project_config.h.
 */

#include <filesystem>
#include <string>

#include "window/window_config.h"
#include "visual_pipeline/visual_pipeline_config.h"

namespace sar {

/**
 * @brief Supported raylib trace log levels loaded from project configuration.
 */
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

/**
 * @brief Fog-of-war visibility algorithm used by the 3D renderer.
 */
enum class Render3DFogMode {
  kCircle,
  kRaycast,
};

/**
 * @brief Returns the configuration name of a 3D fog-of-war mode.
 *
 * @param mode Fog-of-war mode.
 * @return Stable lowercase configuration name.
 */
const char* Render3DFogModeName(Render3DFogMode mode);

/**
 * @brief Controls the small service-information overlay.
 */
struct ServiceInfoConfig {
  bool enabled = true;  ///< true when this configuration block or feature is enabled.
  bool show_memory = true;  ///< Boolean flag controlling show memory.
  int update_interval_ms = 1000;  ///< Time value for update interval milliseconds.
};

/**
 * @brief Controls event-based 3D player movement logging.
 *
 * Logging is intentionally throttled and event-based. It must not write every
 * frame because that would distort runtime performance measurements.
 */
struct Player3DLogConfig {
  bool enabled = true;  ///< true when this configuration block or feature is enabled.
  bool include_mouse = true;  ///< Boolean flag controlling include mouse.
  int tile_log_min_interval_ms = 250;  ///< Time value for tile log min interval milliseconds.
  int blocked_log_min_interval_ms = 600;  ///< Time value for blocked log min interval milliseconds.
};

/**
 * @brief Configures 3D culling, chunk iteration and visible render radius.
 */
struct Render3DPerfConfig {
  int visible_radius_tiles = 48;  ///< Visible radius tiles value carried by this data structure.
  int culling_deadzone_tiles = 4;  ///< Culling deadzone tiles value carried by this data structure.
  int chunk_size_tiles = 16;  ///< Chunk size tiles value carried by this data structure.
  int active_chunk_radius = 3;  ///< Active chunk radius value carried by this data structure.
};

/**
 * @brief Configures 3D visibility radius, fog mode and fog-of-war memory.
 */
struct Render3DVisibilityConfig {
  bool enabled = true;  ///< true when this configuration block or feature is enabled.
  int radius_tiles = 22;  ///< Radius tiles value carried by this data structure.
  bool memory_enabled = true;  ///< Memory enabled value carried by this data structure.
  Render3DFogMode fog_mode = Render3DFogMode::kCircle;  ///< Fog mode value carried by this data structure.
  float seen_tile_dim_factor = 0.32F;  ///< Scaling factor for seen tile dim factor.
};

/**
 * @brief Configures 3D model registry metadata files.
 */
struct Render3DAssetRegistryConfig {
  bool enabled = true;  ///< true when this configuration block or feature is enabled.
  std::filesystem::path asset_library_path =
      "config/render3d/asset_library.json";  ///< Json value carried by this data structure.
  std::filesystem::path tileset_path =
      "config/render3d/tileset_dark_forest.json";  ///< Json value carried by this data structure.
};

/**
 * @brief Configures the 3D new-game camera intro fly-in.
 */
struct Render3DIntroCameraConfig {
  bool enabled = true;  ///< true when this configuration block or feature is enabled.
  int duration_ms = 1800;  ///< Time value for duration milliseconds.
  float start_distance = 42.0F;  ///< Start distance value carried by this data structure.
  float end_distance = 18.0F;  ///< End distance value carried by this data structure.
  float start_height = 20.0F;  ///< Size component for start height.
  float end_height = 10.0F;  ///< Size component for end height.
  float start_yaw_offset_deg = 35.0F;  ///< Start yaw offset deg value carried by this data structure.
  bool lock_player_input = true;  ///< Boolean flag controlling lock player input.
  bool skip_enabled = true;  ///< Boolean flag controlling skip enabled.
};

/**
 * @brief Configures 3D player movement, mouse-facing and jump tuning.
 */
struct Player3DMovementConfig {
  float move_speed_tiles_per_sec = 4.25F;  ///< Time value for move speed tiles per seconds.
  float acceleration_tiles_per_sec2 = 28.0F;  ///< Acceleration tiles per sec2 value carried by this data structure.
  float deceleration_tiles_per_sec2 = 34.0F;  ///< Deceleration tiles per sec2 value carried by this data structure.
  float mouse_turn_sensitivity_rad = 0.0031F;  ///< Mouse turn sensitivity rad value carried by this data structure.
  float movement_multiplier_smooth_speed = 14.0F;  ///< Scaling factor for movement multiplier smooth speed.
  float jump_duration_sec = 0.30F;  ///< Time value for jump duration seconds.
  float jump_arc_elevation_units = 0.62F;  ///< Jump arc elevation units value carried by this data structure.
  float jump_horizontal_speed_multiplier = 1.05F;  ///< Scaling factor for jump horizontal speed multiplier.
  float jump_air_control_multiplier = 0.82F;  ///< Scaling factor for jump air control multiplier.
  float jump_min_running_speed_tiles_per_sec = 1.00F;  ///< Time value for jump min running speed tiles per seconds.
  float standing_speed_multiplier = 1.0F;  ///< Movement speed multiplier while standing.
  float crouched_speed_multiplier = 0.62F;  ///< Movement speed multiplier while crouched.
  float prone_speed_multiplier = 0.32F;  ///< Movement speed multiplier while prone.
  float standing_visibility_factor = 1.0F;  ///< Visibility factor while standing.
  float crouched_visibility_factor = 0.65F;  ///< Visibility factor while crouched.
  float prone_visibility_factor = 0.35F;  ///< Visibility factor while prone.
  float visibility_road_factor = 1.10F;  ///< Visibility factor applied on road terrain.
  float visibility_open_ground_factor = 1.0F;  ///< Visibility factor applied on open ground.
  float visibility_ruins_factor = 0.85F;  ///< Visibility factor applied on ruin terrain.
  float visibility_swamp_factor = 0.80F;  ///< Visibility factor applied on swamp terrain.
  float visibility_water_factor = 0.80F;  ///< Visibility factor applied on water terrain.
  float visibility_forest_factor = 0.72F;  ///< Visibility factor applied on forest terrain.
  float visibility_wall_factor = 0.70F;  ///< Visibility factor applied on wall terrain.
  float visibility_unknown_terrain_factor = 1.0F;  ///< Visibility factor for unknown terrain.
  float visibility_concealment_low_factor = 0.55F;  ///< Visibility factor for low concealment.
  float visibility_concealment_high_factor = 0.42F;  ///< Visibility factor for high concealment.
  float visibility_cover_low_factor = 0.85F;  ///< Visibility factor for low cover.
  float visibility_cover_high_factor = 0.75F;  ///< Visibility factor for high cover.
  float visibility_soft_vision_block_factor = 0.70F;  ///< Visibility factor for soft vision blockers.
  float visibility_below_ground_factor = 0.65F;  ///< Visibility factor below surface elevation.
  float visibility_elevated_factor = 1.08F;  ///< Visibility factor above base elevation.
  float visibility_high_elevation_factor = 1.20F;  ///< Visibility factor on high elevation.
  float visibility_moving_standing_factor = 1.10F;  ///< Moving visibility while standing.
  float visibility_moving_crouched_factor = 1.0F;  ///< Moving visibility while crouched.
  float visibility_moving_prone_factor = 0.95F;  ///< Moving visibility while prone.
  float visibility_min_score = 0.05F;  ///< Minimum clamped player visibility score.
  float visibility_max_score = 2.0F;  ///< Maximum clamped player visibility score.
};


/**
 * @brief Configures 3D player health and fall-damage tuning.
 *
 * The health model is intentionally minimal. It provides a stable runtime
 * contract for fall damage without introducing inventory, healing, death UI,
 * or broader combat systems.
 */
struct Player3DHealthConfig {
  int max_hp = 100;  ///< Maximum health points available to the 3D player.
  int initial_hp = 100;  ///< Health points assigned when a new 3D game starts.
  int fall_damage_per_level = 5;  ///< Damage per unsafe fall level after the first safe level.
};

/**
 * @brief Fully resolved runtime configuration for the application.
 */
struct ProjectConfig {
  std::filesystem::path map_package_path;  ///< Filesystem path used by this configuration or data object.
  std::filesystem::path ui_font_path = "data/fonts/PressStart2P-Regular.ttf";  ///< Filesystem path used by this configuration or data object.
  int ui_font_size = 24;  ///< Ui font size value carried by this data structure.
  RaylibLogLevel raylib_log_level = RaylibLogLevel::kWarning;  ///< Raylib log level value carried by this data structure.
  WindowConfig window_config;  ///< Window config value carried by this data structure.
  ServiceInfoConfig service_info;  ///< Service info value carried by this data structure.
  Player3DLogConfig player3d_log;  ///< Player 3d log value carried by this data structure.
  Render3DPerfConfig render3d_perf;  ///< Render 3d perf value carried by this data structure.
  Render3DVisibilityConfig render3d_visibility;  ///< Render 3d visibility value carried by this data structure.
  Render3DAssetRegistryConfig render3d_assets;  ///< Render 3d assets value carried by this data structure.
  Render3DIntroCameraConfig render3d_intro_camera;  ///< Render 3d intro camera value carried by this data structure.
  Player3DMovementConfig player3d_movement;  ///< Player 3d movement value carried by this data structure.
  Player3DHealthConfig player3d_health;  ///< Player health and fall-damage settings for 3D sessions.
  visual_pipeline::VisualPipelineConfig visual_pipeline_config;  ///< Visual pipeline config value carried by this data structure.

  /**
   * @brief Returns a readable dump of the project configuration.
   *
   * @return String representation for logs and diagnostics.
   */
  std::string Dump() const;
};

/**
 * @brief Result of loading and validating project configuration.
 */
struct ProjectConfigResult {
  bool ok = false;  ///< true when the operation completed successfully.
  ProjectConfig config;  ///< Config value carried by this data structure.
  std::string error;  ///< Human-readable error message when loading or validation fails.
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
