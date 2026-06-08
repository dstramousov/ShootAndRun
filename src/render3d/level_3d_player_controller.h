#ifndef SHOOT_AND_RUN_CPP_SRC_RENDER3D_LEVEL_3D_PLAYER_CONTROLLER_H_
#define SHOOT_AND_RUN_CPP_SRC_RENDER3D_LEVEL_3D_PLAYER_CONTROLLER_H_

#include <cstdint>
#include <string>

#include <raylib.h>

#include "input/input_state.h"
#include "level/level_data.h"
#include "level/terrain_type.h"

namespace sar::render3d {

enum class Level3DMoveBlockReason {
  kNone,
  kOutOfBounds,
  kCollision,
  kNotWalkable,
  kUnderground,
  kStepUpRequired,
  kHeightStep,
};

enum class Level3DJumpEventType {
  kNone,
  kStarted,
  kLanded,
  kBlocked,
};

struct Level3DPlayerState {
  float tile_x = 0.0F;
  float tile_y = 0.0F;
  std::int8_t elevation = 0;
  float facing_x = 0.0F;
  float facing_y = -1.0F;
  float velocity_x_tiles_per_sec = 0.0F;
  float velocity_y_tiles_per_sec = 0.0F;
  float move_speed_tiles_per_sec = 4.25F;
  float current_movement_multiplier = 1.0F;
  float effective_move_speed_tiles_per_sec = 4.25F;
  float acceleration_tiles_per_sec2 = 28.0F;
  float deceleration_tiles_per_sec2 = 34.0F;
  float mouse_turn_sensitivity_rad = 0.0031F;
  int allowed_step_down_height = 1;
  bool step_jump_active = false;
  int step_jump_from_tile_x = -1;
  int step_jump_from_tile_y = -1;
  int step_jump_to_tile_x = -1;
  int step_jump_to_tile_y = -1;
  std::int8_t step_jump_from_elevation = 0;
  std::int8_t step_jump_to_elevation = 0;
  float step_jump_elapsed_sec = 0.0F;
  float step_jump_duration_sec = 0.22F;
  float step_jump_arc_elevation_units = 0.85F;
  float visual_elevation_offset = 0.0F;
  unsigned int jump_event_sequence = 0;
  Level3DJumpEventType last_jump_event_type = Level3DJumpEventType::kNone;
  Level3DMoveBlockReason last_jump_block_reason = Level3DMoveBlockReason::kNone;
  unsigned int transition_event_sequence = 0;
  ElevationTransitionType last_transition_type = ElevationTransitionType::kUnknown;
  int last_transition_from_tile_x = -1;
  int last_transition_from_tile_y = -1;
  int last_transition_to_tile_x = -1;
  int last_transition_to_tile_y = -1;
  std::int8_t last_transition_from_elevation = 0;
  std::int8_t last_transition_to_elevation = 0;
  int last_blocked_tile_x = -1;
  int last_blocked_tile_y = -1;
  Level3DMoveBlockReason last_block_reason = Level3DMoveBlockReason::kNone;
  unsigned int blocked_event_sequence = 0;
  bool initialized = false;
};

struct Level3DPlayerTileDiagnostics {
  int tile_x = -1;
  int tile_y = -1;
  TerrainType terrain = TerrainType::kUnknown;
  bool walkable = false;
  bool collision = false;
  std::uint8_t concealment = 0;
  std::int8_t elevation = 0;
  float movement_multiplier = 0.0F;
  float base_speed_tiles_per_sec = 0.0F;
  float effective_speed_tiles_per_sec = 0.0F;
  float velocity_x_tiles_per_sec = 0.0F;
  float velocity_y_tiles_per_sec = 0.0F;
  float facing_x = 0.0F;
  float facing_y = -1.0F;
};

/**
 * @brief Returns a stable display name for a movement block reason.
 *
 * @param reason Movement block reason.
 * @return Stable lowercase reason name.
 */
const char* Level3DMoveBlockReasonName(Level3DMoveBlockReason reason);


/**
 * @brief Returns a stable display name for a jump event type.
 *
 * @param event_type Jump event type.
 * @return Stable lowercase event name.
 */
const char* Level3DJumpEventTypeName(Level3DJumpEventType event_type);

/**
 * @brief Returns a compact readable dump of the last jump event.
 *
 * @param state Current player state containing the last jump event.
 * @return String representation for event logs.
 */
std::string Level3DJumpEventToString(const Level3DPlayerState& state);

/**
 * @brief Returns a compact readable dump of the last elevation transition event.
 *
 * @param state Current player state containing the last transition event.
 * @return String representation for event logs.
 */
std::string Level3DTransitionEventToString(const Level3DPlayerState& state);

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
 * @brief Updates tile-space 3D player movement from mouse-facing input.
 *
 * Mouse X rotates the player's facing direction. Movement is facing-relative:
 * W/S move forward and backward, while A/D strafe. Runtime collision, height,
 * and movement multiplier data decide whether and how fast the player moves.
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
 * @brief Builds current tile diagnostics for event-based movement logging.
 *
 * @param level Loaded level data.
 * @param state Current player state.
 * @return Current player tile diagnostics.
 */
Level3DPlayerTileDiagnostics CurrentLevel3DPlayerTileDiagnostics(
    const LevelData& level,
    const Level3DPlayerState& state);

/**
 * @brief Returns a readable dump of a 3D player tile diagnostics object.
 *
 * @param diagnostics Current player tile diagnostics.
 * @return String representation for event logs.
 */
std::string Level3DPlayerTileDiagnosticsToString(
    const Level3DPlayerTileDiagnostics& diagnostics);

/**
 * @brief Returns a readable dump of a 3D player state.
 *
 * @param state Current player state.
 * @return String representation for logs and debug overlays.
 */
std::string Level3DPlayerStateToString(const Level3DPlayerState& state);

}  // namespace sar::render3d

#endif  // SHOOT_AND_RUN_CPP_SRC_RENDER3D_LEVEL_3D_PLAYER_CONTROLLER_H_
