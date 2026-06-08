#ifndef SHOOT_AND_RUN_CPP_SRC_RENDER3D_LEVEL_3D_CAMERA_H_
#define SHOOT_AND_RUN_CPP_SRC_RENDER3D_LEVEL_3D_CAMERA_H_

/**
 * @file src/render3d/level_3d_camera.h
 * @brief 3D renderer, camera, player movement, fog, and asset registry. Contains public
 * declarations for level_3d_camera.h.
 */

#include <string>

#include <raylib.h>

#include "input/input_state.h"
#include "level/level_data.h"
#include "render3d/level_3d_player_controller.h"
#include "window/window_state.h"

namespace sar::render3d {

/**
 * @brief Last lifecycle event emitted by the 3D intro camera.
 */
enum class Level3DCameraIntroEvent {
  kNone,
  kStarted,
  kSkipped,
  kFinished,
};

/**
 * @brief Smoothed 3D follow camera state derived from player facing.
 */
struct Level3DCameraState {
  Vector3 position{0.0F, 0.0F, 0.0F};  ///< Position value for position.
  Vector3 target{0.0F, 0.0F, 0.0F};  ///< Target value carried by this data structure.
  Vector3 lookahead{0.0F, 0.0F, 0.0F};  ///< Lookahead value carried by this data structure.
  Vector3 last_forward{0.0F, 0.0F, -1.0F};  ///< Last forward value carried by this data structure.
  float yaw_deg = 270.0F;  ///< Yaw deg value carried by this data structure.
  float height = 18.0F;  ///< Signed elevation level for this tile or object.
  float distance = 42.0F;  ///< Distance value carried by this data structure.
  float distance_target = 42.0F;  ///< Distance target value carried by this data structure.
  float min_distance = 12.0F;  ///< Min distance value carried by this data structure.
  float max_distance = 96.0F;  ///< Max distance value carried by this data structure.
  float zoom_step = 1.5F;  ///< Zoom step value carried by this data structure.
  float zoom_smooth_speed = 12.0F;  ///< Zoom smooth speed value carried by this data structure.
  float follow_smooth_speed = 9.0F;  ///< Follow smooth speed value carried by this data structure.
  float lookahead_smooth_speed = 8.0F;  ///< Lookahead smooth speed value carried by this data structure.
  float forward_smooth_speed = 12.0F;  ///< Forward smooth speed value carried by this data structure.
  float movement_lookahead_tiles = 2.5F;  ///< Movement lookahead tiles value carried by this data structure.
  float target_lookahead_tiles = 2.5F;  ///< Target lookahead tiles value carried by this data structure.
  float bounds_margin_factor = 0.35F;  ///< Scaling factor for bounds margin factor.
  float min_bounds_margin_tiles = 1.0F;  ///< Min bounds margin tiles value carried by this data structure.
  float max_bounds_margin_tiles = 8.0F;  ///< Max bounds margin tiles value carried by this data structure.
  bool intro_enabled = true;  ///< Intro enabled value carried by this data structure.
  bool intro_active = false;  ///< Intro active value carried by this data structure.
  bool intro_finished = false;  ///< Intro finished value carried by this data structure.
  bool intro_lock_player_input = true;  ///< Intro lock player input value carried by this data structure.
  bool intro_skip_enabled = true;  ///< Intro skip enabled value carried by this data structure.
  float intro_elapsed_sec = 0.0F;  ///< Time value for intro elapsed seconds.
  float intro_duration_sec = 1.8F;  ///< Time value for intro duration seconds.
  float intro_start_distance = 42.0F;  ///< Intro start distance value carried by this data structure.
  float intro_end_distance = 18.0F;  ///< Intro end distance value carried by this data structure.
  float intro_start_height = 20.0F;  ///< Size component for intro start height.
  float intro_end_height = 10.0F;  ///< Size component for intro end height.
  float intro_start_yaw_offset_deg = 35.0F;  ///< Intro start yaw offset deg value carried by this data structure.
  Vector3 intro_start_position{0.0F, 0.0F, 0.0F};  ///< Position value for intro start position.
  Vector3 intro_start_target{0.0F, 0.0F, 0.0F};  ///< Intro start target value carried by this data structure.
  Vector3 intro_end_position{0.0F, 0.0F, 0.0F};  ///< Position value for intro end position.
  Vector3 intro_end_target{0.0F, 0.0F, 0.0F};  ///< Intro end target value carried by this data structure.
  Level3DCameraIntroEvent last_intro_event = Level3DCameraIntroEvent::kNone;  ///< Last intro event value carried by this data structure.
  unsigned int intro_event_sequence = 0;  ///< Intro event sequence value carried by this data structure.
  bool initialized = false;  ///< Initialized value carried by this data structure.
};

/**
 * @brief Initializes default 3D camera parameters for the loaded map.
 *
 * @param level Loaded level data.
 * @param state Camera state to initialize.
 */
void InitializeLevel3DCamera(const LevelData& level,
                             Level3DCameraState* state);


/**
 * @brief Starts the configured 3D camera intro fly-in.
 *
 * The intro starts from a wide overview pose and interpolates to the regular
 * follow-camera pose derived from the current player facing direction.
 *
 * @param level Loaded level data.
 * @param player Current player state.
 * @param tile_world_size World units occupied by one map tile.
 * @param elevation_step World-space height of one elevation level.
 * @param state Camera state to update.
 */
void StartLevel3DCameraIntro(const LevelData& level,
                             const Level3DPlayerState& player,
                             float tile_world_size,
                             float elevation_step,
                             Level3DCameraState* state);

/**
 * @brief Returns whether the intro camera currently owns camera movement.
 *
 * @param state Camera state to inspect.
 * @return True when the intro fly-in is active.
 */
bool IsLevel3DCameraIntroActive(const Level3DCameraState& state);

/**
 * @brief Returns whether the active intro should temporarily lock player input.
 *
 * @param state Camera state to inspect.
 * @return True when gameplay movement should be skipped for this frame.
 */
bool Level3DCameraIntroLocksPlayer(const Level3DCameraState& state);

/**
 * @brief Returns whether the current input should skip the active intro.
 *
 * @param state Camera state to inspect.
 * @param input Current frame input state.
 * @return True when the intro can be skipped by user input.
 */
bool Level3DCameraIntroSkipRequested(const Level3DCameraState& state,
                                      const InputState& input);

/**
 * @brief Immediately ends the active intro at the regular follow-camera pose.
 *
 * @param state Camera state to update.
 */
void SkipLevel3DCameraIntro(Level3DCameraState* state);

/**
 * @brief Updates the 3D follow camera from player position and facing.
 *
 * @param level Loaded level data.
 * @param player Current player state.
 * @param input Current input state.
 * @param dt Frame delta time in seconds.
 * @param tile_world_size World units occupied by one map tile.
 * @param elevation_step World-space height of one elevation level.
 * @param state Camera state to update.
 */
void UpdateLevel3DCamera(const LevelData& level,
                         const Level3DPlayerState& player,
                         const InputState& input,
                         float dt,
                         float tile_world_size,
                         float elevation_step,
                         Level3DCameraState* state);

/**
 * @brief Builds a raylib 3D camera that follows the player.
 *
 * @param level Loaded level data.
 * @param player Current player state.
 * @param state Current camera state.
 * @param window Current window state.
 * @param tile_world_size World units occupied by one map tile.
 * @param elevation_step World-space height of one elevation level.
 * @return Raylib 3D camera.
 */
Camera3D BuildLevel3DCamera(const LevelData& level,
                            const Level3DPlayerState& player,
                            const Level3DCameraState& state,
                            const WindowState& window,
                            float tile_world_size,
                            float elevation_step);


/**
 * @brief Returns a compact string for the last intro camera lifecycle event.
 *
 * @param state Current camera state.
 * @return String representation for logs.
 */
std::string Level3DCameraIntroEventToString(
    const Level3DCameraState& state);

/**
 * @brief Returns a readable dump of a 3D camera state.
 *
 * @param state Current camera state.
 * @return String representation for logs and logs.
 */
std::string Level3DCameraStateToString(const Level3DCameraState& state);

}  // namespace sar::render3d

#endif  // SHOOT_AND_RUN_CPP_SRC_RENDER3D_LEVEL_3D_CAMERA_H_
