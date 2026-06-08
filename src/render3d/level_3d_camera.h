#ifndef SHOOT_AND_RUN_CPP_SRC_RENDER3D_LEVEL_3D_CAMERA_H_
#define SHOOT_AND_RUN_CPP_SRC_RENDER3D_LEVEL_3D_CAMERA_H_

#include <string>

#include <raylib.h>

#include "input/input_state.h"
#include "level/level_data.h"
#include "render3d/level_3d_player_controller.h"
#include "window/window_state.h"

namespace sar::render3d {

struct Level3DCameraState {
  Vector3 position{0.0F, 0.0F, 0.0F};
  Vector3 target{0.0F, 0.0F, 0.0F};
  Vector3 lookahead{0.0F, 0.0F, 0.0F};
  Vector3 last_forward{0.0F, 0.0F, -1.0F};
  float yaw_deg = 270.0F;
  float height = 18.0F;
  float distance = 42.0F;
  float distance_target = 42.0F;
  float min_distance = 12.0F;
  float max_distance = 96.0F;
  float zoom_step = 1.5F;
  float zoom_smooth_speed = 12.0F;
  float follow_smooth_speed = 9.0F;
  float lookahead_smooth_speed = 8.0F;
  float forward_smooth_speed = 12.0F;
  float movement_lookahead_tiles = 2.5F;
  float target_lookahead_tiles = 2.5F;
  float bounds_margin_factor = 0.35F;
  float min_bounds_margin_tiles = 1.0F;
  float max_bounds_margin_tiles = 8.0F;
  bool initialized = false;
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
 * @brief Returns a readable dump of a 3D camera state.
 *
 * @param state Current camera state.
 * @return String representation for logs and debug overlays.
 */
std::string Level3DCameraStateToString(const Level3DCameraState& state);

}  // namespace sar::render3d

#endif  // SHOOT_AND_RUN_CPP_SRC_RENDER3D_LEVEL_3D_CAMERA_H_
