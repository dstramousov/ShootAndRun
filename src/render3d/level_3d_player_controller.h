#ifndef SHOOT_AND_RUN_CPP_SRC_RENDER3D_LEVEL_3D_PLAYER_CONTROLLER_H_
#define SHOOT_AND_RUN_CPP_SRC_RENDER3D_LEVEL_3D_PLAYER_CONTROLLER_H_

#include <cstdint>
#include <string>

#include <raylib.h>

#include "input/input_state.h"
#include "level/level_data.h"

namespace sar::render3d {

struct Level3DPlayerState {
  float tile_x = 0.0F;
  float tile_y = 0.0F;
  std::int8_t elevation = 0;
  float move_speed_tiles_per_sec = 4.25F;
  int allowed_step_height = 1;
  bool initialized = false;
};

/**
 * @brief Finds a spawn point and initializes the 3D player state.
 *
 * The function prefers player-spawn markers, then generic spawn markers, and
 * finally falls back to the map center. The returned state uses tile-space
 * coordinates so it remains independent from renderer scale.
 *
 * @param level Loaded level data.
 * @param state Player state to initialize.
 */
void InitializeLevel3DPlayer(const LevelData& level,
                             Level3DPlayerState* state);

/**
 * @brief Updates tile-space 3D player movement from normalized input.
 *
 * Movement uses runtime collision and height data. Blocking cells are rejected,
 * and height differences larger than `allowed_step_height` are rejected until
 * explicit elevation transitions are implemented.
 *
 * @param level Loaded level data.
 * @param input Current input state.
 * @param dt Frame delta time in seconds.
 * @param state Player state to update.
 */
void UpdateLevel3DPlayer(const LevelData& level, const InputState& input,
                         float dt, Level3DPlayerState* state);

/**
 * @brief Converts a tile-space player position into raylib world space.
 *
 * @param level Loaded level data.
 * @param state Current player state.
 * @param tile_world_size World units occupied by one map tile.
 * @param elevation_step World-space height of one elevation level.
 * @return Player position in raylib world coordinates.
 */
Vector3 Level3DPlayerWorldPosition(const LevelData& level,
                                   const Level3DPlayerState& state,
                                   float tile_world_size,
                                   float elevation_step);

/**
 * @brief Returns a readable dump of a 3D player state.
 *
 * @param state Current player state.
 * @return String representation for logs and debug overlays.
 */
std::string Level3DPlayerStateToString(const Level3DPlayerState& state);

}  // namespace sar::render3d

#endif  // SHOOT_AND_RUN_CPP_SRC_RENDER3D_LEVEL_3D_PLAYER_CONTROLLER_H_
