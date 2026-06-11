#ifndef SHOOT_AND_RUN_CPP_SRC_RENDER3D_LEVEL_3D_PLAYER_CONTROLLER_H_
#define SHOOT_AND_RUN_CPP_SRC_RENDER3D_LEVEL_3D_PLAYER_CONTROLLER_H_

/**
 * @file src/render3d/level_3d_player_controller.h
 * @brief 3D renderer, camera, player movement, fog, and asset registry. Contains public
 * declarations for level_3d_player_controller.h.
 */

#include <cstdint>
#include <string>

#include <raylib.h>

#include "input/input_state.h"
#include "level/level_data.h"
#include "level/terrain_type.h"

namespace sar::render3d {

/**
 * @brief Reason why 3D player movement into a target tile was rejected.
 */
enum class Level3DMoveBlockReason {
  kNone,
  kOutOfBounds,
  kCollision,
  kNotWalkable,
  kUnderground,
  kStepUpRequired,
  kHeightStep,
  kPostureCannotClimb,
};

/**
 * @brief Event type emitted by the 3D jump state machine.
 */
enum class Level3DJumpEventType {
  kNone,
  kStarted,
  kLanded,
  kBlocked,
};

/**
 * @brief Jump behavior currently used by the 3D player.
 */
enum class Level3DJumpKind {
  kNone,
  kStepUp,
  kRun,
};

/**
 * @brief Physical body posture used by 3D movement, visibility and rendering.
 */
enum class Level3DPlayerPosture {
  kStanding,
  kCrouched,
  kProne,
};

/**
 * @brief Reason why the 3D player run mode is inactive or rejected.
 */
enum class Level3DRunBlockReason {
  kNone,
  kNotMoving,
  kNotStanding,
  kStaminaTooLow,
  kStaminaEmpty,
  kExhaustedUntilReleased,
  kJumpActive,
};

/**
 * @brief Detailed player visibility factor breakdown.
 */
struct Level3DVisibilityBreakdown {
  float posture_factor = 1.0F;  ///< Visibility factor selected from player posture.
  float terrain_factor = 1.0F;  ///< Visibility factor selected from the current terrain.
  float concealment_factor = 1.0F;  ///< Visibility factor selected from concealment grid.
  float cover_factor = 1.0F;  ///< Visibility factor selected from cover grid.
  float vision_factor = 1.0F;  ///< Visibility factor selected from soft vision blockers.
  float elevation_factor = 1.0F;  ///< Visibility factor selected from current elevation.
  float movement_factor = 1.0F;  ///< Visibility factor selected from current movement.
  float raw_score = 1.0F;  ///< Score before min/max clamping.
  float final_score = 1.0F;  ///< Score after min/max clamping.
};

/**
 * @brief Mutable 3D player movement and jump state.
 *
 * Coordinates are stored in tile units. Visual elevation is stored separately
 * from logical elevation so jumps and step-up transitions can animate without
 * changing the gameplay cell too early.
 */
struct Level3DPlayerState {
  float tile_x = 0.0F;  ///< Tile, screen, or world coordinate for tile x.
  float tile_y = 0.0F;  ///< Tile, screen, or world coordinate for tile y.
  std::int8_t elevation = 0;  ///< Elevation value carried by this data structure.
  float facing_x = 0.0F;  ///< Tile, screen, or world coordinate for facing x.
  float facing_y = -1.0F;  ///< Tile, screen, or world coordinate for facing y.
  float velocity_x_tiles_per_sec = 0.0F;  ///< Time value for velocity x tiles per seconds.
  float velocity_y_tiles_per_sec = 0.0F;  ///< Time value for velocity y tiles per seconds.
  float move_speed_tiles_per_sec = 4.25F;  ///< Time value for move speed tiles per seconds.
  Level3DPlayerPosture posture = Level3DPlayerPosture::kStanding;  ///< Current physical body posture.
  float standing_speed_multiplier = 1.0F;  ///< Movement speed multiplier applied while standing.
  float crouched_speed_multiplier = 0.62F;  ///< Movement speed multiplier applied while crouched.
  float prone_speed_multiplier = 0.32F;  ///< Movement speed multiplier applied while prone.
  float run_speed_multiplier = 1.65F;  ///< Movement speed multiplier applied while running.
  float max_stamina_sec = 3.0F;  ///< Maximum run stamina measured in seconds of running.
  float stamina_sec = 3.0F;  ///< Current run stamina measured in seconds.
  float stamina_drain_per_sec = 1.0F;  ///< Stamina seconds drained per second while running.
  float stamina_recover_per_sec = 0.55F;  ///< Stamina seconds recovered per second while resting.
  float stamina_recover_delay_sec = 0.75F;  ///< Delay before stamina starts recovering after running.
  float stamina_recover_delay_remaining_sec = 0.0F;  ///< Remaining delay before stamina recovery.
  float min_stamina_to_start_run_sec = 0.30F;  ///< Minimum stamina required to start running.
  bool run_active = false;  ///< true when the player is currently running.
  bool run_exhausted_until_released = false;  ///< true after stamina depletion until run input is released.
  Level3DRunBlockReason last_run_block_reason = Level3DRunBlockReason::kNone;  ///< Last reason run was inactive.
  float standing_visibility_factor = 1.0F;  ///< Visibility factor applied while standing.
  float crouched_visibility_factor = 0.65F;  ///< Visibility factor applied while crouched.
  float prone_visibility_factor = 0.35F;  ///< Visibility factor applied while prone.
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
  float visibility_running_factor = 1.25F;  ///< Moving visibility while actively running.
  float visibility_min_score = 0.05F;  ///< Minimum clamped player visibility score.
  float visibility_max_score = 2.0F;  ///< Maximum clamped player visibility score.
  Level3DVisibilityBreakdown visibility;  ///< Detailed current visibility factors.
  float visibility_score = 1.0F;  ///< Current combined visibility score in the range [0, 2].
  float current_movement_multiplier = 1.0F;  ///< Scaling factor for current movement multiplier.
  float target_movement_multiplier = 1.0F;  ///< Scaling factor for target movement multiplier.
  float movement_multiplier_smooth_speed = 14.0F;  ///< Scaling factor for movement multiplier smooth speed.
  float effective_move_speed_tiles_per_sec = 4.25F;  ///< Time value for effective move speed tiles per seconds.
  float acceleration_tiles_per_sec2 = 28.0F;  ///< Acceleration tiles per sec2 value carried by this data structure.
  float deceleration_tiles_per_sec2 = 34.0F;  ///< Deceleration tiles per sec2 value carried by this data structure.
  float mouse_turn_sensitivity_rad = 0.0031F;  ///< Mouse turn sensitivity rad value carried by this data structure.
  int max_hp = 100;  ///< Maximum health points for the current 3D player session.
  int current_hp = 100;  ///< Current health points clamped to the range [0, max_hp].
  int fall_damage_per_level = 5;  ///< Damage per unsafe fall level after the first safe level.
  int allowed_step_down_height = 1;  ///< Height drop that can be walked down without fall damage.
  unsigned int health_event_sequence = 0;  ///< Incremented whenever player health changes.
  int last_health_before_hp = 100;  ///< Health value before the last damage event.
  int last_health_after_hp = 100;  ///< Health value after the last damage event.
  int last_health_damage = 0;  ///< Damage value applied by the last health event.
  unsigned int fall_event_sequence = 0;  ///< Incremented whenever an unsafe downward fall is resolved.
  int last_fall_from_tile_x = -1;  ///< Source tile X for the last fall event.
  int last_fall_from_tile_y = -1;  ///< Source tile Y for the last fall event.
  int last_fall_to_tile_x = -1;  ///< Destination tile X for the last fall event.
  int last_fall_to_tile_y = -1;  ///< Destination tile Y for the last fall event.
  int last_fall_drop_levels = 0;  ///< Number of elevation levels dropped by the last fall event.
  int last_fall_damage = 0;  ///< Damage applied by the last fall event.
  bool jump_active = false;  ///< Jump active value carried by this data structure.
  Level3DJumpKind jump_kind = Level3DJumpKind::kNone;  ///< Jump kind value carried by this data structure.
  float jump_elapsed_sec = 0.0F;  ///< Time value for jump elapsed seconds.
  float jump_duration_sec = 0.32F;  ///< Time value for jump duration seconds.
  float jump_arc_elevation_units = 0.72F;  ///< Jump arc elevation units value carried by this data structure.
  float jump_horizontal_speed_multiplier = 1.08F;  ///< Scaling factor for jump horizontal speed multiplier.
  float jump_air_control_multiplier = 0.72F;  ///< Scaling factor for jump air control multiplier.
  float jump_start_movement_multiplier = 1.0F;  ///< Scaling factor for jump start movement multiplier.
  float jump_min_running_speed_tiles_per_sec = 1.20F;  ///< Time value for jump min running speed tiles per seconds.
  float jump_start_tile_x = 0.0F;  ///< Tile, screen, or world coordinate for jump start tile x.
  float jump_start_tile_y = 0.0F;  ///< Tile, screen, or world coordinate for jump start tile y.
  bool step_jump_active = false;  ///< Step jump active value carried by this data structure.
  int step_jump_from_tile_x = -1;  ///< Tile, screen, or world coordinate for step jump from tile x.
  int step_jump_from_tile_y = -1;  ///< Tile, screen, or world coordinate for step jump from tile y.
  int step_jump_to_tile_x = -1;  ///< Tile, screen, or world coordinate for step jump to tile x.
  int step_jump_to_tile_y = -1;  ///< Tile, screen, or world coordinate for step jump to tile y.
  std::int8_t step_jump_from_elevation = 0;  ///< Step jump from elevation value carried by this data structure.
  std::int8_t step_jump_to_elevation = 0;  ///< Step jump to elevation value carried by this data structure.
  float step_jump_elapsed_sec = 0.0F;  ///< Time value for step jump elapsed seconds.
  float step_jump_duration_sec = 0.22F;  ///< Time value for step jump duration seconds.
  float step_jump_arc_elevation_units = 0.85F;  ///< Step jump arc elevation units value carried by this data structure.
  float visual_elevation_offset = 0.0F;  ///< Visual elevation offset value carried by this data structure.
  unsigned int jump_event_sequence = 0;  ///< Jump event sequence value carried by this data structure.
  Level3DJumpEventType last_jump_event_type = Level3DJumpEventType::kNone;  ///< Semantic type for last jump event.
  Level3DJumpKind last_jump_kind = Level3DJumpKind::kNone;  ///< Last jump kind value carried by this data structure.
  Level3DMoveBlockReason last_jump_block_reason = Level3DMoveBlockReason::kNone;  ///< Last jump block reason value carried by this data structure.
  unsigned int transition_event_sequence = 0;  ///< Transition event sequence value carried by this data structure.
  ElevationTransitionType last_transition_type = ElevationTransitionType::kUnknown;  ///< Semantic type for last transition.
  int last_transition_from_tile_x = -1;  ///< Tile, screen, or world coordinate for last transition from tile x.
  int last_transition_from_tile_y = -1;  ///< Tile, screen, or world coordinate for last transition from tile y.
  int last_transition_to_tile_x = -1;  ///< Tile, screen, or world coordinate for last transition to tile x.
  int last_transition_to_tile_y = -1;  ///< Tile, screen, or world coordinate for last transition to tile y.
  std::int8_t last_transition_from_elevation = 0;  ///< Last transition from elevation value carried by this data structure.
  std::int8_t last_transition_to_elevation = 0;  ///< Last transition to elevation value carried by this data structure.
  int last_blocked_tile_x = -1;  ///< Tile, screen, or world coordinate for last blocked tile x.
  int last_blocked_tile_y = -1;  ///< Tile, screen, or world coordinate for last blocked tile y.
  Level3DMoveBlockReason last_block_reason = Level3DMoveBlockReason::kNone;  ///< Last block reason value carried by this data structure.
  unsigned int blocked_event_sequence = 0;  ///< Blocked event sequence value carried by this data structure.
  bool initialized = false;  ///< Initialized value carried by this data structure.
};

/**
 * @brief Compact diagnostics for the tile currently occupied by the 3D player.
 */
struct Level3DPlayerTileDiagnostics {
  int tile_x = -1;  ///< Tile, screen, or world coordinate for tile x.
  int tile_y = -1;  ///< Tile, screen, or world coordinate for tile y.
  TerrainType terrain = TerrainType::kUnknown;  ///< Base terrain classification for this runtime cell.
  bool walkable = false;  ///< true when movement is allowed by the movement grid.
  bool collision = false;  ///< true when this cell blocks physical movement.
  std::uint8_t concealment = 0;  ///< Concealment strength encoded by the runtime grid.
  std::int8_t elevation = 0;  ///< Elevation value carried by this data structure.
  float movement_multiplier = 0.0F;  ///< Movement speed multiplier applied on this tile.
  float base_speed_tiles_per_sec = 0.0F;  ///< Time value for base speed tiles per seconds.
  Level3DPlayerPosture posture = Level3DPlayerPosture::kStanding;  ///< Current player body posture.
  float posture_speed_multiplier = 1.0F;  ///< Movement multiplier applied by the current posture.
  bool run_active = false;  ///< true when run mode is active.
  Level3DRunBlockReason run_block_reason = Level3DRunBlockReason::kNone;  ///< Last reason run is inactive.
  float run_speed_multiplier = 1.0F;  ///< Movement multiplier applied by active run mode.
  float stamina_sec = 0.0F;  ///< Current run stamina in seconds.
  float max_stamina_sec = 0.0F;  ///< Maximum run stamina in seconds.
  float stamina_recover_delay_remaining_sec = 0.0F;  ///< Remaining stamina recovery delay.
  Level3DVisibilityBreakdown visibility;  ///< Detailed current visibility factors.
  float visibility_score = 1.0F;  ///< Current combined player visibility score.
  float effective_speed_tiles_per_sec = 0.0F;  ///< Time value for effective speed tiles per seconds.
  float velocity_x_tiles_per_sec = 0.0F;  ///< Time value for velocity x tiles per seconds.
  float velocity_y_tiles_per_sec = 0.0F;  ///< Time value for velocity y tiles per seconds.
  float facing_x = 0.0F;  ///< Tile, screen, or world coordinate for facing x.
  float facing_y = -1.0F;  ///< Tile, screen, or world coordinate for facing y.
};


/**
 * @brief Compact diagnostics for the tile currently faced by the 3D player.
 */
struct Level3DTargetTileDiagnostics {
  int from_tile_x = -1;  ///< Source tile X used for the movement probe.
  int from_tile_y = -1;  ///< Source tile Y used for the movement probe.
  int target_tile_x = -1;  ///< Target tile X selected from the current facing direction.
  int target_tile_y = -1;  ///< Target tile Y selected from the current facing direction.
  TerrainType terrain = TerrainType::kUnknown;  ///< Base terrain classification for the target tile.
  bool walkable = false;  ///< true when movement is allowed by the target movement grid.
  bool collision = false;  ///< true when the target cell blocks physical movement.
  bool has_transition = false;  ///< true when an explicit elevation transition connects source and target.
  bool can_enter = false;  ///< true when normal movement can enter the target tile.
  bool can_space_step = false;  ///< true when Space can step up into the target tile.
  std::int8_t from_elevation = 0;  ///< Source tile elevation.
  std::int8_t target_elevation = 0;  ///< Target tile elevation.
  int height_delta = 0;  ///< Target elevation minus source elevation.
  float movement_multiplier = 0.0F;  ///< Movement multiplier encoded by the target tile.
  ElevationTransitionType transition_type = ElevationTransitionType::kUnknown;  ///< Explicit transition type, if present.
  Level3DMoveBlockReason reason = Level3DMoveBlockReason::kNone;  ///< Normal movement block reason.
};

/**
 * @brief Returns a stable display name for a movement block reason.
 *
 * @param reason Movement block reason.
 * @return Stable lowercase reason name.
 */
const char* Level3DMoveBlockReasonName(Level3DMoveBlockReason reason);


/**
 * @brief Returns a stable display name for a run block reason.
 *
 * @param reason Run block reason.
 * @return Stable lowercase reason name.
 */
const char* Level3DRunBlockReasonName(Level3DRunBlockReason reason);

/**
 * @brief Returns a stable display name for a player posture.
 *
 * @param posture Player body posture.
 * @return Stable lowercase posture name.
 */
const char* Level3DPlayerPostureName(Level3DPlayerPosture posture);

/**
 * @brief Returns a stable display name for a jump kind.
 *
 * @param kind Jump kind.
 * @return Stable lowercase jump kind name.
 */
const char* Level3DJumpKindName(Level3DJumpKind kind);

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
 * @brief Returns a compact readable dump of the last fall event.
 *
 * @param state Current player state containing the last fall event.
 * @return String representation for event logs.
 */
std::string Level3DFallEventToString(const Level3DPlayerState& state);

/**
 * @brief Returns a compact readable dump of the last health change event.
 *
 * @param state Current player state containing the last health change event.
 * @return String representation for event logs.
 */
std::string Level3DHealthEventToString(const Level3DPlayerState& state);

/**
 * @brief Recomputes the current player visibility score from posture and tile context.
 *
 * @param level Loaded level data.
 * @param state Player state to update.
 */
void RefreshLevel3DPlayerVisibility(const LevelData& level,
                                    Level3DPlayerState* state);

/**
 * @brief Computes detailed visibility factors for the current player tile.
 *
 * @param level Loaded level data.
 * @param state Current player state and visibility tuning.
 * @return Detailed visibility factor breakdown.
 */
Level3DVisibilityBreakdown CurrentLevel3DPlayerVisibilityBreakdown(
    const LevelData& level, const Level3DPlayerState& state);

/**
 * @brief Formats a visibility breakdown for diagnostics and debug overlays.
 *
 * @param breakdown Visibility breakdown to format.
 * @return Compact human-readable diagnostics string.
 */
std::string Level3DVisibilityBreakdownToString(
    const Level3DVisibilityBreakdown& breakdown);

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
 * @brief Builds diagnostics for the tile currently faced by the 3D player.
 *
 * @param level Loaded level data.
 * @param state Current player state.
 * @return Target tile diagnostics for overlays and event-based logs.
 */
Level3DTargetTileDiagnostics FacingLevel3DTargetTileDiagnostics(
    const LevelData& level,
    const Level3DPlayerState& state);

/**
 * @brief Returns a readable dump of a 3D target tile diagnostics object.
 *
 * @param diagnostics Target tile diagnostics.
 * @return String representation for event logs and debug overlays.
 */
std::string Level3DTargetTileDiagnosticsToString(
    const Level3DTargetTileDiagnostics& diagnostics);

/**
 * @brief Returns a readable dump of a 3D player state.
 *
 * @param state Current player state.
 * @return String representation for logs and debug overlays.
 */
std::string Level3DPlayerStateToString(const Level3DPlayerState& state);

}  // namespace sar::render3d

#endif  // SHOOT_AND_RUN_CPP_SRC_RENDER3D_LEVEL_3D_PLAYER_CONTROLLER_H_
