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
  float yaw_deg = 45.0F;
  float pitch_deg = 55.0F;
  float distance = 42.0F;
  float min_distance = 12.0F;
  float max_distance = 96.0F;
  float rotate_speed_deg_per_sec = 95.0F;
  float zoom_step = 1.10F;
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
 * @brief Updates 3D camera yaw and distance from user input.
 *
 * @param input Current input state.
 * @param dt Frame delta time in seconds.
 * @param state Camera state to update.
 */
void UpdateLevel3DCamera(const InputState& input, float dt,
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
