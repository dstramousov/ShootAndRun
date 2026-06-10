/**
 * @file src/render3d/level_3d_player_controller.cpp
 * @brief 3D renderer, camera, player movement, fog, and asset registry. Contains implementation
 * for level_3d_player_controller.cpp.
 */

#include "render3d/level_3d_player_controller.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string_view>

namespace sar::render3d {
namespace {

constexpr float kVectorEpsilon = 0.0001F;
constexpr float kPi = 3.14159265358979323846F;

/**
 * @brief Stores enter tile result data shared between runtime systems.
 */
struct EnterTileResult {
  bool can_enter = false;
  int tile_x = -1;
  int tile_y = -1;
  int height_delta = 0;
  Level3DMoveBlockReason reason = Level3DMoveBlockReason::kNone;
  bool used_transition = false;
  ElevationTransitionType transition_type = ElevationTransitionType::kUnknown;
  bool used_running_jump = false;
};

/**
 * @brief Checks whether a string contains a requested substring.
 */
bool TextContains(std::string_view text, std::string_view needle) {
  return text.find(needle) != std::string_view::npos;
}

/**
 * @brief Checks whether preferred spawn marker is true.
 */
bool IsPreferredSpawnMarker(const Marker& marker) {
  return marker.type == "player_spawn" || marker.id == "player_spawn" ||
         TextContains(marker.type, "player_spawn") ||
         TextContains(marker.id, "player_spawn");
}

/**
 * @brief Checks whether fallback spawn marker is true.
 */
bool IsFallbackSpawnMarker(const Marker& marker) {
  return marker.type == "start" || marker.id == "start" ||
         TextContains(marker.type, "spawn") || TextContains(marker.id, "spawn");
}

/**
 * @brief Finds spawn marker.
 */
const Marker* FindSpawnMarker(const LevelData& level) {
  for (const Marker& marker : level.markers) {
    if (IsPreferredSpawnMarker(marker)) {
      return &marker;
    }
  }

  for (const Marker& marker : level.markers) {
    if (IsFallbackSpawnMarker(marker)) {
      return &marker;
    }
  }

  return nullptr;
}

/**
 * @brief Checks whether tile coordinates are inside the loaded map.
 */
bool IsInsideMap(const LevelData& level, int x, int y) {
  return x >= 0 && y >= 0 && x < level.size.width && y < level.size.height;
}

/**
 * @brief Returns the runtime cell at tile coordinates or nullptr when outside the map.
 */
const RuntimeCell* CellAt(const LevelData& level, int x, int y) {
  if (!IsInsideMap(level, x, y)) {
    return nullptr;
  }
  const int index = y * level.size.width + x;
  if (index < 0 || index >= static_cast<int>(level.cells.size())) {
    return nullptr;
  }
  return &level.cells[static_cast<std::size_t>(index)];
}

/**
 * @brief Executes the height at or zero operation.
 */
std::int8_t HeightAtOrZero(const LevelData& level, int x, int y) {
  const RuntimeCell* cell = CellAt(level, x, y);
  return cell != nullptr ? cell->height : 0;
}

/**
 * @brief Converts a floating tile position to an integer tile index.
 */
int TileIndexFromPosition(float value) {
  return static_cast<int>(std::floor(value));
}

/**
 * @brief Sets initial facing toward map center.
 */
void SetInitialFacingTowardMapCenter(const LevelData& level,
                                     Level3DPlayerState* state) {
  if (state == nullptr || level.size.width <= 0 || level.size.height <= 0) {
    return;
  }

  const float center_x = static_cast<float>(level.size.width) * 0.5F;
  const float center_y = static_cast<float>(level.size.height) * 0.5F;
  const float direction_x = center_x - state->tile_x;
  const float direction_y = center_y - state->tile_y;
  const float length = std::hypot(direction_x, direction_y);
  if (length <= kVectorEpsilon) {
    state->facing_x = 0.0F;
    state->facing_y = -1.0F;
    return;
  }

  state->facing_x = direction_x / length;
  state->facing_y = direction_y / length;
}

/**
 * @brief Returns terrain short name.
 */
std::string_view TerrainShortName(TerrainType terrain) {
  switch (terrain) {
    case TerrainType::kOpenGround:
      return "open";
    case TerrainType::kForest:
      return "forest";
    case TerrainType::kRoad:
      return "road";
    case TerrainType::kSwamp:
      return "swamp";
    case TerrainType::kRuins:
      return "ruins";
    case TerrainType::kWater:
      return "water";
    case TerrainType::kWall:
      return "wall";
    case TerrainType::kUnknown:
      return "unknown";
  }
  return "unknown";
}

/**
 * @brief Returns posture speed multiplier from player state tuning.
 */
float PostureSpeedMultiplier(const Level3DPlayerState& state) {
  switch (state.posture) {
    case Level3DPlayerPosture::kStanding:
      return state.standing_speed_multiplier;
    case Level3DPlayerPosture::kCrouched:
      return state.crouched_speed_multiplier;
    case Level3DPlayerPosture::kProne:
      return state.prone_speed_multiplier;
  }
  return 1.0F;
}

/**
 * @brief Returns posture visibility factor from player state tuning.
 */
float PostureVisibilityFactor(const Level3DPlayerState& state) {
  switch (state.posture) {
    case Level3DPlayerPosture::kStanding:
      return state.standing_visibility_factor;
    case Level3DPlayerPosture::kCrouched:
      return state.crouched_visibility_factor;
    case Level3DPlayerPosture::kProne:
      return state.prone_visibility_factor;
  }
  return 1.0F;
}

/**
 * @brief Returns terrain contribution to player visibility.
 */
float TerrainVisibilityFactor(const Level3DPlayerState& state,
                              TerrainType terrain) {
  switch (terrain) {
    case TerrainType::kRoad:
      return state.visibility_road_factor;
    case TerrainType::kOpenGround:
      return state.visibility_open_ground_factor;
    case TerrainType::kRuins:
      return state.visibility_ruins_factor;
    case TerrainType::kSwamp:
      return state.visibility_swamp_factor;
    case TerrainType::kWater:
      return state.visibility_water_factor;
    case TerrainType::kForest:
      return state.visibility_forest_factor;
    case TerrainType::kWall:
      return state.visibility_wall_factor;
    case TerrainType::kUnknown:
      return state.visibility_unknown_terrain_factor;
  }
  return state.visibility_unknown_terrain_factor;
}

/**
 * @brief Returns concealment contribution to player visibility.
 */
float ConcealmentVisibilityFactor(const Level3DPlayerState& state,
                                  const RuntimeCell& cell) {
  if (cell.concealment <= 0) {
    return 1.0F;
  }
  return cell.concealment >= 2 ? state.visibility_concealment_high_factor
                               : state.visibility_concealment_low_factor;
}

/**
 * @brief Returns cover contribution to player visibility.
 */
float CoverVisibilityFactor(const Level3DPlayerState& state,
                            const RuntimeCell& cell) {
  if (cell.cover <= 0) {
    return 1.0F;
  }
  return cell.cover >= 2 ? state.visibility_cover_high_factor
                         : state.visibility_cover_low_factor;
}

/**
 * @brief Returns soft vision-blocking contribution to player visibility.
 */
float VisionVisibilityFactor(const Level3DPlayerState& state,
                             const RuntimeCell& cell) {
  if (cell.blocks_vision && !cell.collision) {
    return state.visibility_soft_vision_block_factor;
  }
  return 1.0F;
}

/**
 * @brief Returns elevation contribution to player visibility.
 */
float ElevationVisibilityFactor(const Level3DPlayerState& state,
                                const RuntimeCell& cell) {
  const int elevation = std::min(static_cast<int>(state.elevation),
                                 static_cast<int>(cell.height));
  if (elevation < 0) {
    return state.visibility_below_ground_factor;
  }
  if (elevation >= 3) {
    return state.visibility_high_elevation_factor;
  }
  if (elevation > 0) {
    return state.visibility_elevated_factor;
  }
  return 1.0F;
}

/**
 * @brief Returns movement contribution to player visibility.
 */
float MovementVisibilityFactor(const Level3DPlayerState& state) {
  const float speed = std::hypot(state.velocity_x_tiles_per_sec,
                                 state.velocity_y_tiles_per_sec);
  if (speed <= 0.05F) {
    return 1.0F;
  }
  switch (state.posture) {
    case Level3DPlayerPosture::kStanding:
      return state.visibility_moving_standing_factor;
    case Level3DPlayerPosture::kCrouched:
      return state.visibility_moving_crouched_factor;
    case Level3DPlayerPosture::kProne:
      return state.visibility_moving_prone_factor;
  }
  return 1.0F;
}

/**
 * @brief Returns a fully populated visibility breakdown.
 */
Level3DVisibilityBreakdown BuildVisibilityBreakdown(
    const Level3DPlayerState& state, const RuntimeCell* cell) {
  Level3DVisibilityBreakdown breakdown;
  breakdown.posture_factor = PostureVisibilityFactor(state);

  if (cell == nullptr) {
    breakdown.raw_score = breakdown.posture_factor;
    breakdown.final_score = std::clamp(breakdown.raw_score,
                                       state.visibility_min_score,
                                       state.visibility_max_score);
    return breakdown;
  }

  breakdown.terrain_factor = TerrainVisibilityFactor(state, cell->terrain);
  breakdown.concealment_factor = ConcealmentVisibilityFactor(state, *cell);
  breakdown.cover_factor = CoverVisibilityFactor(state, *cell);
  breakdown.vision_factor = VisionVisibilityFactor(state, *cell);
  breakdown.elevation_factor = ElevationVisibilityFactor(state, *cell);
  breakdown.movement_factor = MovementVisibilityFactor(state);
  breakdown.raw_score = breakdown.posture_factor * breakdown.terrain_factor *
                        breakdown.concealment_factor *
                        breakdown.cover_factor * breakdown.vision_factor *
                        breakdown.elevation_factor *
                        breakdown.movement_factor;
  breakdown.final_score = std::clamp(breakdown.raw_score,
                                     state.visibility_min_score,
                                     state.visibility_max_score);
  return breakdown;
}

/**
 * @brief Checks whether upward movement is blocked by the current posture.
 */
bool PostureBlocksUpwardMovement(const Level3DPlayerState& state,
                                 int height_delta) {
  return state.posture == Level3DPlayerPosture::kProne && height_delta > 0;
}

/**
 * @brief Checks whether running jumps are allowed for the current posture.
 */
bool CanStartRunningJumpFromPosture(const Level3DPlayerState& state) {
  return state.posture == Level3DPlayerPosture::kStanding;
}

/**
 * @brief Applies posture toggle input to the player state.
 */
void ApplyPostureInput(const InputState& input, Level3DPlayerState* state) {
  if (state == nullptr || state->jump_active || state->step_jump_active) {
    return;
  }

  if (input.prone_pressed) {
    state->posture = state->posture == Level3DPlayerPosture::kProne
                         ? Level3DPlayerPosture::kCrouched
                         : Level3DPlayerPosture::kProne;
    return;
  }

  if (input.crouch_pressed) {
    switch (state->posture) {
      case Level3DPlayerPosture::kStanding:
        state->posture = Level3DPlayerPosture::kCrouched;
        break;
      case Level3DPlayerPosture::kCrouched:
        state->posture = Level3DPlayerPosture::kStanding;
        break;
      case Level3DPlayerPosture::kProne:
        state->posture = Level3DPlayerPosture::kCrouched;
        break;
    }
  }
}

/**
 * @brief Returns current tile movement multiplier.
 */
float CurrentTileMovementMultiplier(const LevelData& level,
                                    const Level3DPlayerState& state) {
  const RuntimeCell* cell = CellAt(level, TileIndexFromPosition(state.tile_x),
                                   TileIndexFromPosition(state.tile_y));
  if (cell == nullptr || cell->collision || !cell->walkable) {
    return 0.0F;
  }
  return std::clamp(cell->movement_multiplier, 0.0F, 1.50F);
}

/**
 * @brief Executes the movement multiplier for state operation.
 */
float MovementMultiplierForState(const LevelData& level,
                                 const Level3DPlayerState& state) {
  if (state.jump_active && state.jump_kind == Level3DJumpKind::kRun) {
    return std::clamp(state.jump_start_movement_multiplier, 0.0F, 1.50F);
  }
  return CurrentTileMovementMultiplier(level, state);
}

/**
 * @brief Executes the exponential alpha operation.
 */
float ExponentialAlpha(float speed, float dt) {
  if (speed <= 0.0F || dt <= 0.0F) {
    return 1.0F;
  }
  return std::clamp(1.0F - std::exp(-speed * dt), 0.0F, 1.0F);
}

/**
 * @brief Executes the refresh effective movement speed operation.
 */
void RefreshEffectiveMovementSpeed(const LevelData& level,
                                   Level3DPlayerState* state) {
  if (state == nullptr) {
    return;
  }
  state->target_movement_multiplier = MovementMultiplierForState(level, *state);
  state->effective_move_speed_tiles_per_sec =
      state->move_speed_tiles_per_sec * state->current_movement_multiplier *
      std::clamp(PostureSpeedMultiplier(*state), 0.0F, 1.50F);
}

/**
 * @brief Resets movement multiplier to defaults.
 */
void ResetMovementMultiplier(const LevelData& level,
                             Level3DPlayerState* state) {
  if (state == nullptr) {
    return;
  }
  state->target_movement_multiplier = CurrentTileMovementMultiplier(
      level, *state);
  state->current_movement_multiplier = state->target_movement_multiplier;
  state->effective_move_speed_tiles_per_sec =
      state->move_speed_tiles_per_sec * state->current_movement_multiplier *
      std::clamp(PostureSpeedMultiplier(*state), 0.0F, 1.50F);
}

/**
 * @brief Updates movement multiplier for the current frame.
 */
void UpdateMovementMultiplier(const LevelData& level, float safe_dt,
                              Level3DPlayerState* state) {
  if (state == nullptr) {
    return;
  }

  state->target_movement_multiplier = MovementMultiplierForState(level, *state);
  const float alpha = ExponentialAlpha(state->movement_multiplier_smooth_speed,
                                       safe_dt);
  state->current_movement_multiplier +=
      (state->target_movement_multiplier - state->current_movement_multiplier) *
      alpha;
  state->current_movement_multiplier = std::clamp(
      state->current_movement_multiplier, 0.0F, 1.50F);
  state->effective_move_speed_tiles_per_sec =
      state->move_speed_tiles_per_sec * state->current_movement_multiplier *
      std::clamp(PostureSpeedMultiplier(*state), 0.0F, 1.50F);
}

/**
 * @brief Returns record blocked tile.
 */
void RecordBlockedTile(int tile_x, int tile_y,
                       Level3DMoveBlockReason reason,
                       Level3DPlayerState* state) {
  if (state == nullptr || reason == Level3DMoveBlockReason::kNone) {
    return;
  }
  if (state->last_blocked_tile_x == tile_x &&
      state->last_blocked_tile_y == tile_y &&
      state->last_block_reason == reason) {
    return;
  }

  state->last_blocked_tile_x = tile_x;
  state->last_blocked_tile_y = tile_y;
  state->last_block_reason = reason;
  ++state->blocked_event_sequence;
}

/**
 * @brief Executes the facing relative input direction operation.
 */
void FacingRelativeInputDirection(const InputState& input,
                                  const Level3DPlayerState& state,
                                  float* out_x,
                                  float* out_y);

/**
 * @brief Checks whether same transition endpoint is true.
 */
bool IsSameTransitionEndpoint(const ElevationTransition& transition,
                              int from_x, int from_y, int to_x, int to_y) {
  if (transition.from_x == from_x && transition.from_y == from_y &&
      transition.to_x == to_x && transition.to_y == to_y) {
    return true;
  }
  return transition.bidirectional && transition.from_x == to_x &&
         transition.from_y == to_y && transition.to_x == from_x &&
         transition.to_y == from_y;
}

/**
 * @brief Finds elevation transition.
 */
const ElevationTransition* FindElevationTransition(const LevelData& level,
                                                   int from_x, int from_y,
                                                   int to_x, int to_y) {
  for (const ElevationTransition& transition : level.elevation_transitions) {
    if (IsSameTransitionEndpoint(transition, from_x, from_y, to_x, to_y)) {
      return &transition;
    }
  }
  return nullptr;
}

/**
 * @brief Executes the can use normal movement transition operation.
 */
bool CanUseNormalMovementTransition(const ElevationTransition& transition,
                                    std::int8_t from_height,
                                    std::int8_t to_height) {
  const int height_delta = static_cast<int>(to_height) -
                           static_cast<int>(from_height);
  switch (transition.type) {
    case ElevationTransitionType::kRamp:
    case ElevationTransitionType::kStairs:
      return std::abs(height_delta) <= 1;
    case ElevationTransitionType::kHatch:
      return false;
    case ElevationTransitionType::kStep:
    case ElevationTransitionType::kUnknown:
      return false;
  }
  return false;
}

/**
 * @brief Executes the can use step jump transition operation.
 */
bool CanUseStepJumpTransition(const ElevationTransition& transition,
                              int height_delta) {
  if (height_delta != 1) {
    return false;
  }
  switch (transition.type) {
    case ElevationTransitionType::kStep:
    case ElevationTransitionType::kUnknown:
      return true;
    case ElevationTransitionType::kRamp:
    case ElevationTransitionType::kStairs:
    case ElevationTransitionType::kHatch:
      return false;
  }
  return false;
}

/**
 * @brief Executes the record transition event operation.
 */
void RecordTransitionEvent(const EnterTileResult& enter_result,
                           const Level3DPlayerState& previous_state,
                           Level3DPlayerState* state) {
  if (state == nullptr || !enter_result.used_transition) {
    return;
  }
  state->last_transition_type = enter_result.transition_type;
  state->last_transition_from_tile_x = TileIndexFromPosition(previous_state.tile_x);
  state->last_transition_from_tile_y = TileIndexFromPosition(previous_state.tile_y);
  state->last_transition_to_tile_x = enter_result.tile_x;
  state->last_transition_to_tile_y = enter_result.tile_y;
  state->last_transition_from_elevation = previous_state.elevation;
  state->last_transition_to_elevation = static_cast<std::int8_t>(
      static_cast<int>(previous_state.elevation) + enter_result.height_delta);
  ++state->transition_event_sequence;
}

/**
 * @brief Returns current horizontal speed.
 */
float CurrentHorizontalSpeed(const Level3DPlayerState& state) {
  return std::hypot(state.velocity_x_tiles_per_sec,
                    state.velocity_y_tiles_per_sec);
}

/**
 * @brief Checks whether running jump candidate is true.
 */
bool IsRunningJumpCandidate(const Level3DPlayerState& state) {
  return state.jump_active && state.jump_kind == Level3DJumpKind::kRun &&
         CurrentHorizontalSpeed(state) >= state.jump_min_running_speed_tiles_per_sec;
}

/**
 * @brief Returns check enter tile.
 */
EnterTileResult CheckEnterTile(const LevelData& level,
                               const Level3DPlayerState& state,
                               float next_x,
                               float next_y) {
  const int x = TileIndexFromPosition(next_x);
  const int y = TileIndexFromPosition(next_y);
  const RuntimeCell* target = CellAt(level, x, y);
  if (target == nullptr) {
    return {false, x, y, 0, Level3DMoveBlockReason::kOutOfBounds};
  }

  const int height_delta = static_cast<int>(target->height) -
                           static_cast<int>(state.elevation);
  const int current_x = TileIndexFromPosition(state.tile_x);
  const int current_y = TileIndexFromPosition(state.tile_y);
  const ElevationTransition* transition = FindElevationTransition(
      level, current_x, current_y, x, y);
  if (target->collision) {
    return {false, x, y, height_delta, Level3DMoveBlockReason::kCollision};
  }
  if (!target->walkable || target->movement_multiplier <= 0.0F) {
    return {false, x, y, height_delta, Level3DMoveBlockReason::kNotWalkable};
  }
  if (PostureBlocksUpwardMovement(state, height_delta)) {
    return {false, x, y, height_delta,
            Level3DMoveBlockReason::kPostureCannotClimb};
  }
  if (transition != nullptr && state.elevation >= 0 && target->height >= 0 &&
      CanUseNormalMovementTransition(*transition, state.elevation,
                                     target->height)) {
    return {true, x, y, height_delta, Level3DMoveBlockReason::kNone,
            true, transition->type};
  }
  if (height_delta == 1 && CanStartRunningJumpFromPosture(state) &&
      IsRunningJumpCandidate(state)) {
    return {true, x, y, height_delta, Level3DMoveBlockReason::kNone,
            false, ElevationTransitionType::kUnknown, true};
  }
  if (height_delta > 0) {
    const Level3DMoveBlockReason reason =
        height_delta == 1 ? Level3DMoveBlockReason::kStepUpRequired
                          : Level3DMoveBlockReason::kHeightStep;
    return {false, x, y, height_delta, reason};
  }
  return {true, x, y, height_delta, Level3DMoveBlockReason::kNone};
}

/**
 * @brief Clamps 01 to a safe range.
 */
float Clamp01(float value) {
  return std::clamp(value, 0.0F, 1.0F);
}

/**
 * @brief Executes the smooth step01 operation.
 */
float SmoothStep01(float value) {
  const float clamped = Clamp01(value);
  return clamped * clamped * (3.0F - 2.0F * clamped);
}

/**
 * @brief Executes the record jump event operation.
 */
void RecordJumpEvent(Level3DJumpEventType event_type,
                     Level3DMoveBlockReason block_reason,
                     Level3DPlayerState* state) {
  if (state == nullptr || event_type == Level3DJumpEventType::kNone) {
    return;
  }

  state->last_jump_event_type = event_type;
  state->last_jump_kind = state->jump_kind;
  if (state->last_jump_kind == Level3DJumpKind::kNone &&
      event_type == Level3DJumpEventType::kBlocked) {
    state->last_jump_kind = Level3DJumpKind::kStepUp;
  }
  state->last_jump_block_reason = block_reason;
  ++state->jump_event_sequence;
}

/**
 * @brief Records fall and health events for unsafe downward elevation drops.
 */
void ApplyFallDamageIfNeeded(const Level3DPlayerState& previous_state,
                             int target_tile_x, int target_tile_y,
                             std::int8_t target_elevation,
                             Level3DPlayerState* state) {
  if (state == nullptr) {
    return;
  }

  const int drop_levels = static_cast<int>(previous_state.elevation) -
                          static_cast<int>(target_elevation);
  if (drop_levels <= state->allowed_step_down_height) {
    return;
  }

  const int unsafe_levels = std::max(0, drop_levels -
                                           state->allowed_step_down_height);
  const int damage = unsafe_levels * std::max(0, state->fall_damage_per_level);
  state->last_fall_from_tile_x = TileIndexFromPosition(previous_state.tile_x);
  state->last_fall_from_tile_y = TileIndexFromPosition(previous_state.tile_y);
  state->last_fall_to_tile_x = target_tile_x;
  state->last_fall_to_tile_y = target_tile_y;
  state->last_fall_drop_levels = drop_levels;
  state->last_fall_damage = damage;
  ++state->fall_event_sequence;

  if (damage <= 0) {
    return;
  }

  state->last_health_before_hp = state->current_hp;
  state->last_health_damage = damage;
  state->current_hp = std::clamp(state->current_hp - damage, 0,
                                 std::max(0, state->max_hp));
  state->last_health_after_hp = state->current_hp;
  ++state->health_event_sequence;
}

/**
 * @brief Executes the step direction from input operation.
 */
bool StepDirectionFromInput(const InputState& input,
                            const Level3DPlayerState& state,
                            int* out_step_x,
                            int* out_step_y) {
  if (out_step_x == nullptr || out_step_y == nullptr) {
    return false;
  }

  float direction_x = 0.0F;
  float direction_y = 0.0F;
  FacingRelativeInputDirection(input, state, &direction_x, &direction_y);
  if (std::abs(direction_x) <= kVectorEpsilon &&
      std::abs(direction_y) <= kVectorEpsilon) {
    return false;
  }

  *out_step_x = 0;
  *out_step_y = 0;
  if (std::abs(direction_x) >= std::abs(direction_y)) {
    *out_step_x = direction_x > 0.0F ? 1 : -1;
  } else {
    *out_step_y = direction_y > 0.0F ? 1 : -1;
  }
  return true;
}

/**
 * @brief Returns a single-tile step matching the current facing direction.
 */
bool FacingStep(const Level3DPlayerState& state, int* out_step_x,
                int* out_step_y) {
  if (out_step_x == nullptr || out_step_y == nullptr) {
    return false;
  }
  *out_step_x = 0;
  *out_step_y = 0;
  if (std::abs(state.facing_x) <= kVectorEpsilon &&
      std::abs(state.facing_y) <= kVectorEpsilon) {
    *out_step_y = -1;
    return true;
  }
  if (std::abs(state.facing_x) >= std::abs(state.facing_y)) {
    *out_step_x = state.facing_x >= 0.0F ? 1 : -1;
  } else {
    *out_step_y = state.facing_y >= 0.0F ? 1 : -1;
  }
  return true;
}

/**
 * @brief Executes the check step jump target operation.
 */
EnterTileResult CheckStepJumpTarget(const LevelData& level,
                                    const Level3DPlayerState& state,
                                    int target_x,
                                    int target_y) {
  const RuntimeCell* target = CellAt(level, target_x, target_y);
  if (target == nullptr) {
    return {false, target_x, target_y, 0,
            Level3DMoveBlockReason::kOutOfBounds};
  }

  const int height_delta = static_cast<int>(target->height) -
                           static_cast<int>(state.elevation);
  const int current_x = TileIndexFromPosition(state.tile_x);
  const int current_y = TileIndexFromPosition(state.tile_y);
  const ElevationTransition* transition = FindElevationTransition(
      level, current_x, current_y, target_x, target_y);
  if (target->collision) {
    return {false, target_x, target_y, height_delta,
            Level3DMoveBlockReason::kCollision};
  }
  if (!target->walkable || target->movement_multiplier <= 0.0F) {
    return {false, target_x, target_y, height_delta,
            Level3DMoveBlockReason::kNotWalkable};
  }
  if (PostureBlocksUpwardMovement(state, height_delta)) {
    return {false, target_x, target_y, height_delta,
            Level3DMoveBlockReason::kPostureCannotClimb};
  }
  if (transition != nullptr && state.elevation >= 0 && target->height >= 0 &&
      !CanUseStepJumpTransition(*transition, height_delta)) {
    return {false, target_x, target_y, height_delta,
            Level3DMoveBlockReason::kNone};
  }
  if (height_delta == 1) {
    return {true, target_x, target_y, height_delta,
            Level3DMoveBlockReason::kNone};
  }
  if (height_delta > 1) {
    return {false, target_x, target_y, height_delta,
            Level3DMoveBlockReason::kHeightStep};
  }

  return {false, target_x, target_y, height_delta,
          Level3DMoveBlockReason::kNone};
}

/**
 * @brief Starts step jump.
 */
void StartStepJump(const EnterTileResult& target,
                   Level3DPlayerState* state) {
  if (state == nullptr || !target.can_enter) {
    return;
  }

  state->jump_active = true;
  state->jump_kind = Level3DJumpKind::kStepUp;
  state->jump_elapsed_sec = 0.0F;
  state->jump_start_movement_multiplier = state->current_movement_multiplier;
  state->jump_start_tile_x = state->tile_x;
  state->jump_start_tile_y = state->tile_y;
  state->step_jump_active = true;
  state->step_jump_from_tile_x = TileIndexFromPosition(state->tile_x);
  state->step_jump_from_tile_y = TileIndexFromPosition(state->tile_y);
  state->step_jump_to_tile_x = target.tile_x;
  state->step_jump_to_tile_y = target.tile_y;
  state->step_jump_from_elevation = state->elevation;
  state->step_jump_to_elevation = static_cast<std::int8_t>(
      static_cast<int>(state->elevation) + target.height_delta);
  state->step_jump_elapsed_sec = 0.0F;
  state->visual_elevation_offset = 0.0F;
  state->velocity_x_tiles_per_sec = 0.0F;
  state->velocity_y_tiles_per_sec = 0.0F;
  RecordJumpEvent(Level3DJumpEventType::kStarted,
                  Level3DMoveBlockReason::kNone, state);
}

/**
 * @brief Starts running jump.
 */
void StartRunningJump(Level3DPlayerState* state) {
  if (state == nullptr || state->jump_active || state->step_jump_active) {
    return;
  }

  state->jump_active = true;
  state->jump_kind = Level3DJumpKind::kRun;
  state->jump_elapsed_sec = 0.0F;
  state->jump_start_movement_multiplier = std::max(
      state->current_movement_multiplier, 0.0F);
  state->jump_start_tile_x = state->tile_x;
  state->jump_start_tile_y = state->tile_y;
  state->visual_elevation_offset = 0.0F;
  state->step_jump_from_tile_x = TileIndexFromPosition(state->tile_x);
  state->step_jump_from_tile_y = TileIndexFromPosition(state->tile_y);
  state->step_jump_to_tile_x = state->step_jump_from_tile_x;
  state->step_jump_to_tile_y = state->step_jump_from_tile_y;
  state->step_jump_from_elevation = state->elevation;
  state->step_jump_to_elevation = state->elevation;
  RecordJumpEvent(Level3DJumpEventType::kStarted,
                  Level3DMoveBlockReason::kNone, state);
}

/**
 * @brief Executes the try start running jump operation.
 */
bool TryStartRunningJump(const LevelData& level, const InputState& input,
                         Level3DPlayerState* state) {
  if (state == nullptr || !input.jump_pressed || state->jump_active ||
      state->step_jump_active || !CanStartRunningJumpFromPosture(*state)) {
    return false;
  }

  RefreshEffectiveMovementSpeed(level, state);
  StartRunningJump(state);
  return true;
}

/**
 * @brief Executes the try start step jump operation.
 */
bool TryStartStepJump(const LevelData& level, const InputState& input,
                      Level3DPlayerState* state) {
  if (state == nullptr || !input.jump_pressed || state->jump_active ||
      state->step_jump_active) {
    return false;
  }

  int step_x = 0;
  int step_y = 0;
  if (!StepDirectionFromInput(input, *state, &step_x, &step_y)) {
    return false;
  }

  const int current_x = TileIndexFromPosition(state->tile_x);
  const int current_y = TileIndexFromPosition(state->tile_y);
  const EnterTileResult target = CheckStepJumpTarget(
      level, *state, current_x + step_x, current_y + step_y);
  if (target.can_enter) {
    if (CurrentHorizontalSpeed(*state) <
        state->jump_min_running_speed_tiles_per_sec) {
      StartStepJump(target, state);
      return true;
    }
    return false;
  }

  if (target.reason != Level3DMoveBlockReason::kNone) {
    state->step_jump_from_tile_x = current_x;
    state->step_jump_from_tile_y = current_y;
    state->step_jump_to_tile_x = target.tile_x;
    state->step_jump_to_tile_y = target.tile_y;
    state->step_jump_from_elevation = state->elevation;
    state->step_jump_to_elevation = static_cast<std::int8_t>(
        static_cast<int>(state->elevation) + target.height_delta);
    RecordJumpEvent(Level3DJumpEventType::kBlocked, target.reason, state);
    RecordBlockedTile(target.tile_x, target.tile_y, target.reason, state);
    return true;
  }

  return false;
}

/**
 * @brief Updates step jump for the current frame.
 */
bool UpdateStepJump(const LevelData& level, float safe_dt,
                    Level3DPlayerState* state) {
  if (state == nullptr || !state->step_jump_active) {
    return false;
  }

  state->step_jump_elapsed_sec += safe_dt;
  state->jump_elapsed_sec = state->step_jump_elapsed_sec;
  const float duration = std::max(state->step_jump_duration_sec, 0.001F);
  const float progress = Clamp01(state->step_jump_elapsed_sec / duration);
  const float smooth_progress = SmoothStep01(progress);

  const float from_x = static_cast<float>(state->step_jump_from_tile_x) + 0.5F;
  const float from_y = static_cast<float>(state->step_jump_from_tile_y) + 0.5F;
  const float to_x = static_cast<float>(state->step_jump_to_tile_x) + 0.5F;
  const float to_y = static_cast<float>(state->step_jump_to_tile_y) + 0.5F;
  state->tile_x = from_x + (to_x - from_x) * smooth_progress;
  state->tile_y = from_y + (to_y - from_y) * smooth_progress;

  const float elevation_delta = static_cast<float>(
      static_cast<int>(state->step_jump_to_elevation) -
      static_cast<int>(state->step_jump_from_elevation));
  const float arc = std::sin(kPi * progress) *
                    state->step_jump_arc_elevation_units;
  state->visual_elevation_offset = elevation_delta * smooth_progress + arc;

  if (progress < 1.0F) {
    return true;
  }

  state->tile_x = to_x;
  state->tile_y = to_y;
  state->elevation = state->step_jump_to_elevation;
  state->visual_elevation_offset = 0.0F;
  RecordJumpEvent(Level3DJumpEventType::kLanded,
                  Level3DMoveBlockReason::kNone, state);
  state->step_jump_active = false;
  state->jump_active = false;
  state->jump_kind = Level3DJumpKind::kNone;
  state->step_jump_elapsed_sec = 0.0F;
  state->jump_elapsed_sec = 0.0F;
  RefreshEffectiveMovementSpeed(level, state);
  return true;
}

/**
 * @brief Executes the landing block reason operation.
 */
Level3DMoveBlockReason LandingBlockReason(const LevelData& level,
                                          int tile_x, int tile_y) {
  const RuntimeCell* cell = CellAt(level, tile_x, tile_y);
  if (cell == nullptr) {
    return Level3DMoveBlockReason::kOutOfBounds;
  }
  if (cell->collision) {
    return Level3DMoveBlockReason::kCollision;
  }
  if (!cell->walkable || cell->movement_multiplier <= 0.0F) {
    return Level3DMoveBlockReason::kNotWalkable;
  }
  return Level3DMoveBlockReason::kNone;
}

/**
 * @brief Resets jump state to defaults.
 */
void ResetJumpState(Level3DPlayerState* state) {
  if (state == nullptr) {
    return;
  }
  state->jump_active = false;
  state->jump_kind = Level3DJumpKind::kNone;
  state->jump_elapsed_sec = 0.0F;
  state->step_jump_active = false;
  state->step_jump_elapsed_sec = 0.0F;
}

/**
 * @brief Updates running jump for the current frame.
 */
bool UpdateRunningJump(const LevelData& level, float safe_dt,
                       Level3DPlayerState* state) {
  if (state == nullptr || !state->jump_active ||
      state->jump_kind != Level3DJumpKind::kRun) {
    return false;
  }

  state->jump_elapsed_sec += safe_dt;
  const float duration = std::max(state->jump_duration_sec, 0.001F);
  const float progress = Clamp01(state->jump_elapsed_sec / duration);
  const float smooth_progress = SmoothStep01(progress);
  const float elevation_delta = static_cast<float>(
      static_cast<int>(state->step_jump_to_elevation) -
      static_cast<int>(state->step_jump_from_elevation));
  const float arc = std::sin(kPi * progress) *
                    state->jump_arc_elevation_units;
  state->visual_elevation_offset = elevation_delta * smooth_progress + arc;

  if (progress < 1.0F) {
    return true;
  }

  const int landing_x = TileIndexFromPosition(state->tile_x);
  const int landing_y = TileIndexFromPosition(state->tile_y);
  const Level3DMoveBlockReason landing_reason = LandingBlockReason(
      level, landing_x, landing_y);
  if (landing_reason != Level3DMoveBlockReason::kNone) {
    state->tile_x = state->jump_start_tile_x;
    state->tile_y = state->jump_start_tile_y;
    state->elevation = state->step_jump_from_elevation;
    state->step_jump_to_tile_x = landing_x;
    state->step_jump_to_tile_y = landing_y;
    state->step_jump_to_elevation = HeightAtOrZero(level, landing_x, landing_y);
    state->visual_elevation_offset = 0.0F;
    RecordJumpEvent(Level3DJumpEventType::kBlocked, landing_reason, state);
    RecordBlockedTile(landing_x, landing_y, landing_reason, state);
    ResetJumpState(state);
    RefreshEffectiveMovementSpeed(level, state);
    return true;
  }

  Level3DPlayerState jump_start_state = *state;
  jump_start_state.tile_x = state->jump_start_tile_x;
  jump_start_state.tile_y = state->jump_start_tile_y;
  jump_start_state.elevation = state->step_jump_from_elevation;
  state->elevation = HeightAtOrZero(level, landing_x, landing_y);
  state->step_jump_to_tile_x = landing_x;
  state->step_jump_to_tile_y = landing_y;
  state->step_jump_to_elevation = state->elevation;
  state->visual_elevation_offset = 0.0F;
  ApplyFallDamageIfNeeded(jump_start_state, landing_x, landing_y,
                          state->elevation, state);
  RecordJumpEvent(Level3DJumpEventType::kLanded,
                  Level3DMoveBlockReason::kNone, state);
  ResetJumpState(state);
  RefreshEffectiveMovementSpeed(level, state);
  return true;
}

/**
 * @brief Applies movement axis.
 */
void ApplyMovementAxis(const LevelData& level, float dx, float dy,
                       Level3DPlayerState* state) {
  if (state == nullptr) {
    return;
  }

  const float next_x = std::clamp(state->tile_x + dx, 0.0F,
                                  static_cast<float>(level.size.width) -
                                      0.001F);
  const float next_y = std::clamp(state->tile_y + dy, 0.0F,
                                  static_cast<float>(level.size.height) -
                                      0.001F);
  const EnterTileResult enter_result = CheckEnterTile(level, *state,
                                                      next_x, next_y);
  if (!enter_result.can_enter) {
    RecordBlockedTile(enter_result.tile_x, enter_result.tile_y,
                      enter_result.reason, state);
    return;
  }

  const Level3DPlayerState previous_state = *state;
  state->tile_x = next_x;
  state->tile_y = next_y;
  const std::int8_t target_elevation = HeightAtOrZero(
      level, TileIndexFromPosition(state->tile_x),
      TileIndexFromPosition(state->tile_y));
  if (state->jump_active && state->jump_kind == Level3DJumpKind::kRun) {
    state->step_jump_to_tile_x = enter_result.tile_x;
    state->step_jump_to_tile_y = enter_result.tile_y;
    state->step_jump_to_elevation = target_elevation;
  } else {
    state->elevation = target_elevation;
    ApplyFallDamageIfNeeded(previous_state, enter_result.tile_x,
                            enter_result.tile_y, target_elevation, state);
  }
  RefreshEffectiveMovementSpeed(level, state);
  RecordTransitionEvent(enter_result, previous_state, state);
}

/**
 * @brief Executes the normalize facing operation.
 */
void NormalizeFacing(Level3DPlayerState* state) {
  if (state == nullptr) {
    return;
  }
  const float length = std::hypot(state->facing_x, state->facing_y);
  if (length <= kVectorEpsilon) {
    state->facing_x = 0.0F;
    state->facing_y = -1.0F;
    return;
  }
  state->facing_x /= length;
  state->facing_y /= length;
}

/**
 * @brief Updates facing from mouse for the current frame.
 */
void UpdateFacingFromMouse(const InputState& input,
                           Level3DPlayerState* state) {
  if (state == nullptr || std::abs(input.mouse_delta.x) <= kVectorEpsilon) {
    return;
  }

  const float current_angle = std::atan2(state->facing_y, state->facing_x);
  const float new_angle = current_angle + input.mouse_delta.x *
                                              state->mouse_turn_sensitivity_rad;
  state->facing_x = std::cos(new_angle);
  state->facing_y = std::sin(new_angle);
  NormalizeFacing(state);
}

/**
 * @brief Executes the facing relative input direction operation.
 */
void FacingRelativeInputDirection(const InputState& input,
                                  const Level3DPlayerState& state,
                                  float* out_x,
                                  float* out_y) {
  if (out_x == nullptr || out_y == nullptr) {
    return;
  }

  float move_x = 0.0F;
  float move_y = 0.0F;
  if (input.left_down) {
    move_x -= 1.0F;
  }
  if (input.right_down) {
    move_x += 1.0F;
  }
  if (input.up_down) {
    move_y -= 1.0F;
  }
  if (input.down_down) {
    move_y += 1.0F;
  }

  if (move_x == 0.0F && move_y == 0.0F) {
    *out_x = 0.0F;
    *out_y = 0.0F;
    return;
  }

  const float facing_length = std::hypot(state.facing_x, state.facing_y);
  const float forward_x = facing_length > kVectorEpsilon
                              ? state.facing_x / facing_length
                              : 0.0F;
  const float forward_y = facing_length > kVectorEpsilon
                              ? state.facing_y / facing_length
                              : -1.0F;
  const float right_x = -forward_y;
  const float right_y = forward_x;

  float desired_x = right_x * move_x + forward_x * (-move_y);
  float desired_y = right_y * move_x + forward_y * (-move_y);
  const float desired_length = std::hypot(desired_x, desired_y);
  if (desired_length <= kVectorEpsilon) {
    *out_x = 0.0F;
    *out_y = 0.0F;
    return;
  }

  desired_x /= desired_length;
  desired_y /= desired_length;
  *out_x = desired_x;
  *out_y = desired_y;
}

/**
 * @brief Executes the move velocity toward operation.
 */
void MoveVelocityToward(float target_x, float target_y, float max_delta,
                        Level3DPlayerState* state) {
  if (state == nullptr) {
    return;
  }

  const float delta_x = target_x - state->velocity_x_tiles_per_sec;
  const float delta_y = target_y - state->velocity_y_tiles_per_sec;
  const float delta_length = std::hypot(delta_x, delta_y);
  if (delta_length <= max_delta || delta_length <= kVectorEpsilon) {
    state->velocity_x_tiles_per_sec = target_x;
    state->velocity_y_tiles_per_sec = target_y;
    return;
  }

  const float ratio = max_delta / delta_length;
  state->velocity_x_tiles_per_sec += delta_x * ratio;
  state->velocity_y_tiles_per_sec += delta_y * ratio;
}

/**
 * @brief Updates velocity from input for the current frame.
 */
void UpdateVelocityFromInput(const LevelData& level,
                             const InputState& input,
                             float safe_dt,
                             Level3DPlayerState* state) {
  if (state == nullptr) {
    return;
  }

  UpdateMovementMultiplier(level, safe_dt, state);

  float direction_x = 0.0F;
  float direction_y = 0.0F;
  FacingRelativeInputDirection(input, *state, &direction_x, &direction_y);

  float effective_speed = state->effective_move_speed_tiles_per_sec;
  float acceleration = (direction_x == 0.0F && direction_y == 0.0F)
                           ? state->deceleration_tiles_per_sec2
                           : state->acceleration_tiles_per_sec2;
  if (state->jump_active && state->jump_kind == Level3DJumpKind::kRun) {
    effective_speed = state->move_speed_tiles_per_sec *
                      state->jump_start_movement_multiplier *
                      std::clamp(PostureSpeedMultiplier(*state), 0.0F, 1.50F) *
                      state->jump_horizontal_speed_multiplier;
    acceleration *= state->jump_air_control_multiplier;
  }

  const float target_x = direction_x * effective_speed;
  const float target_y = direction_y * effective_speed;
  MoveVelocityToward(target_x, target_y, acceleration * safe_dt, state);
}

}  // namespace

/**
 * @brief Returns level 3D move block reason name.
 */
const char* Level3DMoveBlockReasonName(Level3DMoveBlockReason reason) {
  switch (reason) {
    case Level3DMoveBlockReason::kNone:
      return "none";
    case Level3DMoveBlockReason::kOutOfBounds:
      return "out_of_bounds";
    case Level3DMoveBlockReason::kCollision:
      return "collision";
    case Level3DMoveBlockReason::kNotWalkable:
      return "not_walkable";
    case Level3DMoveBlockReason::kUnderground:
      return "underground";
    case Level3DMoveBlockReason::kStepUpRequired:
      return "step_up_required";
    case Level3DMoveBlockReason::kHeightStep:
      return "height_step";
    case Level3DMoveBlockReason::kPostureCannotClimb:
      return "posture_cannot_climb";
  }
  return "unknown";
}

/**
 * @brief Returns level 3D player posture name.
 */
const char* Level3DPlayerPostureName(Level3DPlayerPosture posture) {
  switch (posture) {
    case Level3DPlayerPosture::kStanding:
      return "standing";
    case Level3DPlayerPosture::kCrouched:
      return "crouched";
    case Level3DPlayerPosture::kProne:
      return "prone";
  }
  return "unknown";
}

/**
 * @brief Returns level 3D jump kind name.
 */
const char* Level3DJumpKindName(Level3DJumpKind kind) {
  switch (kind) {
    case Level3DJumpKind::kNone:
      return "none";
    case Level3DJumpKind::kStepUp:
      return "step_up";
    case Level3DJumpKind::kRun:
      return "run";
  }
  return "unknown";
}

/**
 * @brief Returns level 3D jump event type name.
 */
const char* Level3DJumpEventTypeName(Level3DJumpEventType event_type) {
  switch (event_type) {
    case Level3DJumpEventType::kNone:
      return "none";
    case Level3DJumpEventType::kStarted:
      return "jump_start";
    case Level3DJumpEventType::kLanded:
      return "jump_land";
    case Level3DJumpEventType::kBlocked:
      return "jump_block";
  }
  return "unknown";
}

/**
 * @brief Initializes level 3D player.
 */
void InitializeLevel3DPlayer(const LevelData& level,
                             Level3DPlayerState* state) {
  if (state == nullptr) {
    return;
  }

  const Marker* marker = FindSpawnMarker(level);
  if (marker != nullptr) {
    state->tile_x = static_cast<float>(marker->x) + 0.5F;
    state->tile_y = static_cast<float>(marker->y) + 0.5F;
    state->elevation = HeightAtOrZero(level, marker->x, marker->y);
  } else {
    state->tile_x = static_cast<float>(level.size.width) * 0.5F;
    state->tile_y = static_cast<float>(level.size.height) * 0.5F;
    state->elevation = HeightAtOrZero(
        level, TileIndexFromPosition(state->tile_x),
        TileIndexFromPosition(state->tile_y));
  }
  SetInitialFacingTowardMapCenter(level, state);
  state->velocity_x_tiles_per_sec = 0.0F;
  state->velocity_y_tiles_per_sec = 0.0F;
  state->current_hp = std::clamp(state->current_hp, 0,
                                  std::max(0, state->max_hp));
  state->last_health_before_hp = state->current_hp;
  state->last_health_after_hp = state->current_hp;
  state->last_health_damage = 0;
  state->health_event_sequence = 0;
  state->fall_event_sequence = 0;
  state->last_fall_from_tile_x = -1;
  state->last_fall_from_tile_y = -1;
  state->last_fall_to_tile_x = -1;
  state->last_fall_to_tile_y = -1;
  state->last_fall_drop_levels = 0;
  state->last_fall_damage = 0;
  state->last_blocked_tile_x = -1;
  state->last_blocked_tile_y = -1;
  state->last_block_reason = Level3DMoveBlockReason::kNone;
  state->blocked_event_sequence = 0;
  state->jump_active = false;
  state->jump_kind = Level3DJumpKind::kNone;
  state->jump_elapsed_sec = 0.0F;
  state->jump_start_movement_multiplier = 1.0F;
  state->jump_start_tile_x = state->tile_x;
  state->jump_start_tile_y = state->tile_y;
  state->step_jump_active = false;
  state->step_jump_from_tile_x = -1;
  state->step_jump_from_tile_y = -1;
  state->step_jump_to_tile_x = -1;
  state->step_jump_to_tile_y = -1;
  state->step_jump_from_elevation = state->elevation;
  state->step_jump_to_elevation = state->elevation;
  state->step_jump_elapsed_sec = 0.0F;
  state->visual_elevation_offset = 0.0F;
  state->jump_event_sequence = 0;
  state->last_jump_event_type = Level3DJumpEventType::kNone;
  state->last_jump_kind = Level3DJumpKind::kNone;
  state->last_jump_block_reason = Level3DMoveBlockReason::kNone;
  state->transition_event_sequence = 0;
  state->last_transition_type = ElevationTransitionType::kUnknown;
  state->last_transition_from_tile_x = -1;
  state->last_transition_from_tile_y = -1;
  state->last_transition_to_tile_x = -1;
  state->last_transition_to_tile_y = -1;
  state->last_transition_from_elevation = state->elevation;
  state->last_transition_to_elevation = state->elevation;
  state->posture = Level3DPlayerPosture::kStanding;
  ResetMovementMultiplier(level, state);
  RefreshLevel3DPlayerVisibility(level, state);
  state->initialized = true;
}

/**
 * @brief Updates level 3D player for the current frame.
 */
void UpdateLevel3DPlayer(const LevelData& level, const InputState& input,
                         float dt, Level3DPlayerState* state) {
  if (state == nullptr || !state->initialized || level.size.width <= 0 ||
      level.size.height <= 0) {
    return;
  }

  const float safe_dt = std::clamp(dt, 0.0F, 0.05F);
  UpdateFacingFromMouse(input, state);
  ApplyPostureInput(input, state);
  RefreshLevel3DPlayerVisibility(level, state);
  if (UpdateStepJump(level, safe_dt, state)) {
    RefreshLevel3DPlayerVisibility(level, state);
    return;
  }
  const bool running_jump_active = UpdateRunningJump(level, safe_dt, state);
  if (TryStartStepJump(level, input, state)) {
    RefreshLevel3DPlayerVisibility(level, state);
    return;
  }
  if (!running_jump_active) {
    TryStartRunningJump(level, input, state);
  }
  UpdateVelocityFromInput(level, input, safe_dt, state);

  const float dx = state->velocity_x_tiles_per_sec * safe_dt;
  const float dy = state->velocity_y_tiles_per_sec * safe_dt;
  if (std::abs(dx) <= kVectorEpsilon && std::abs(dy) <= kVectorEpsilon) {
    RefreshLevel3DPlayerVisibility(level, state);
    return;
  }

  const EnterTileResult full_enter = CheckEnterTile(
      level, *state, state->tile_x + dx, state->tile_y + dy);
  if (full_enter.can_enter) {
    ApplyMovementAxis(level, dx, dy, state);
    RefreshLevel3DPlayerVisibility(level, state);
    return;
  }

  ApplyMovementAxis(level, dx, 0.0F, state);
  ApplyMovementAxis(level, 0.0F, dy, state);
  if (std::abs(dx) > kVectorEpsilon || std::abs(dy) > kVectorEpsilon) {
    RecordBlockedTile(full_enter.tile_x, full_enter.tile_y,
                      full_enter.reason, state);
  }
  RefreshLevel3DPlayerVisibility(level, state);
}

/**
 * @brief Refreshes level 3D player visibility score.
 */
void RefreshLevel3DPlayerVisibility(const LevelData& level,
                                    Level3DPlayerState* state) {
  if (state == nullptr) {
    return;
  }

  state->visibility = CurrentLevel3DPlayerVisibilityBreakdown(level, *state);
  state->visibility_score = state->visibility.final_score;
}

/**
 * @brief Returns current detailed level 3D player visibility factors.
 */
Level3DVisibilityBreakdown CurrentLevel3DPlayerVisibilityBreakdown(
    const LevelData& level, const Level3DPlayerState& state) {
  const RuntimeCell* cell = CellAt(level, TileIndexFromPosition(state.tile_x),
                                   TileIndexFromPosition(state.tile_y));
  return BuildVisibilityBreakdown(state, cell);
}

/**
 * @brief Executes the level 3D player world position operation.
 */
Vector3 Level3DPlayerWorldPosition(const LevelData& level,
                                   const Level3DPlayerState& state,
                                   float tile_world_size,
                                   float elevation_step) {
  const float origin_x = static_cast<float>(level.size.width) * tile_world_size *
                         0.5F;
  const float origin_z = static_cast<float>(level.size.height) *
                         tile_world_size * 0.5F;
  const float visual_elevation = static_cast<float>(state.elevation) +
                                 state.visual_elevation_offset;
  return Vector3{state.tile_x * tile_world_size - origin_x,
                 visual_elevation * elevation_step,
                 state.tile_y * tile_world_size - origin_z};
}

/**
 * @brief Returns current level 3D player tile diagnostics.
 */
Level3DPlayerTileDiagnostics CurrentLevel3DPlayerTileDiagnostics(
    const LevelData& level,
    const Level3DPlayerState& state) {
  Level3DPlayerTileDiagnostics diagnostics;
  diagnostics.tile_x = TileIndexFromPosition(state.tile_x);
  diagnostics.tile_y = TileIndexFromPosition(state.tile_y);
  diagnostics.base_speed_tiles_per_sec = state.move_speed_tiles_per_sec;
  diagnostics.posture = state.posture;
  diagnostics.posture_speed_multiplier =
      std::clamp(PostureSpeedMultiplier(state), 0.0F, 1.50F);
  diagnostics.visibility = CurrentLevel3DPlayerVisibilityBreakdown(level, state);
  diagnostics.visibility_score = diagnostics.visibility.final_score;
  diagnostics.effective_speed_tiles_per_sec =
      state.effective_move_speed_tiles_per_sec;
  diagnostics.velocity_x_tiles_per_sec = state.velocity_x_tiles_per_sec;
  diagnostics.velocity_y_tiles_per_sec = state.velocity_y_tiles_per_sec;
  diagnostics.facing_x = state.facing_x;
  diagnostics.facing_y = state.facing_y;

  const RuntimeCell* cell = CellAt(level, diagnostics.tile_x,
                                   diagnostics.tile_y);
  if (cell == nullptr) {
    return diagnostics;
  }

  diagnostics.terrain = cell->terrain;
  diagnostics.walkable = cell->walkable;
  diagnostics.collision = cell->collision;
  diagnostics.concealment = cell->concealment;
  diagnostics.elevation = cell->height;
  diagnostics.movement_multiplier = state.current_movement_multiplier;
  return diagnostics;
}

/**
 * @brief Builds facing target diagnostics.
 */
Level3DTargetTileDiagnostics FacingLevel3DTargetTileDiagnostics(
    const LevelData& level,
    const Level3DPlayerState& state) {
  Level3DTargetTileDiagnostics diagnostics;
  diagnostics.from_tile_x = TileIndexFromPosition(state.tile_x);
  diagnostics.from_tile_y = TileIndexFromPosition(state.tile_y);
  diagnostics.from_elevation = state.elevation;

  int step_x = 0;
  int step_y = 0;
  if (!FacingStep(state, &step_x, &step_y)) {
    diagnostics.target_tile_x = diagnostics.from_tile_x;
    diagnostics.target_tile_y = diagnostics.from_tile_y;
    return diagnostics;
  }

  diagnostics.target_tile_x = diagnostics.from_tile_x + step_x;
  diagnostics.target_tile_y = diagnostics.from_tile_y + step_y;
  const RuntimeCell* target = CellAt(level, diagnostics.target_tile_x,
                                     diagnostics.target_tile_y);
  if (target != nullptr) {
    diagnostics.terrain = target->terrain;
    diagnostics.walkable = target->walkable;
    diagnostics.collision = target->collision;
    diagnostics.target_elevation = target->height;
    diagnostics.movement_multiplier = target->movement_multiplier;
  }
  diagnostics.height_delta = static_cast<int>(diagnostics.target_elevation) -
                             static_cast<int>(diagnostics.from_elevation);

  const ElevationTransition* transition = FindElevationTransition(
      level, diagnostics.from_tile_x, diagnostics.from_tile_y,
      diagnostics.target_tile_x, diagnostics.target_tile_y);
  if (transition != nullptr) {
    diagnostics.has_transition = true;
    diagnostics.transition_type = transition->type;
  }

  const EnterTileResult enter_result = CheckEnterTile(
      level, state, static_cast<float>(diagnostics.target_tile_x) + 0.5F,
      static_cast<float>(diagnostics.target_tile_y) + 0.5F);
  diagnostics.can_enter = enter_result.can_enter;
  diagnostics.reason = enter_result.reason;

  const EnterTileResult step_result = CheckStepJumpTarget(
      level, state, diagnostics.target_tile_x, diagnostics.target_tile_y);
  diagnostics.can_space_step = step_result.can_enter;
  return diagnostics;
}

/**
 * @brief Returns level 3D target tile diagnostics to string.
 */
std::string Level3DTargetTileDiagnosticsToString(
    const Level3DTargetTileDiagnostics& diagnostics) {
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(2);
  stream << "target=" << diagnostics.target_tile_x << ','
         << diagnostics.target_tile_y
         << " from=" << diagnostics.from_tile_x << ','
         << diagnostics.from_tile_y
         << " ter=" << TerrainShortName(diagnostics.terrain)
         << " el=" << static_cast<int>(diagnostics.target_elevation)
         << " delta=" << diagnostics.height_delta
         << " w=" << (diagnostics.walkable ? 'Y' : 'N')
         << " col=" << (diagnostics.collision ? 'Y' : 'N')
         << " mov=" << diagnostics.movement_multiplier
         << " transition="
         << (diagnostics.has_transition
                 ? ElevationTransitionTypeName(diagnostics.transition_type)
                 : "none")
         << " normal=" << (diagnostics.can_enter ? "allowed" : "blocked")
         << " reason=" << Level3DMoveBlockReasonName(diagnostics.reason)
         << " space=" << (diagnostics.can_space_step ? "allowed" : "blocked");
  return stream.str();
}

/**
 * @brief Returns level 3D jump event to string.
 */
std::string Level3DJumpEventToString(const Level3DPlayerState& state) {
  std::ostringstream stream;
  stream << Level3DJumpEventTypeName(state.last_jump_event_type)
         << " kind=" << Level3DJumpKindName(state.last_jump_kind)
         << " from=" << state.step_jump_from_tile_x << ','
         << state.step_jump_from_tile_y
         << " el=" << static_cast<int>(state.step_jump_from_elevation)
         << " to=" << state.step_jump_to_tile_x << ','
         << state.step_jump_to_tile_y
         << " el=" << static_cast<int>(state.step_jump_to_elevation);
  if (state.last_jump_event_type == Level3DJumpEventType::kBlocked) {
    stream << " reason="
           << Level3DMoveBlockReasonName(state.last_jump_block_reason);
  }
  return stream.str();
}

/**
 * @brief Returns level 3D transition event to string.
 */
std::string Level3DTransitionEventToString(
    const Level3DPlayerState& state) {
  std::ostringstream stream;
  stream << "transition type="
         << ElevationTransitionTypeName(state.last_transition_type)
         << " from=" << state.last_transition_from_tile_x << ','
         << state.last_transition_from_tile_y
         << " el=" << static_cast<int>(state.last_transition_from_elevation)
         << " to=" << state.last_transition_to_tile_x << ','
         << state.last_transition_to_tile_y
         << " el=" << static_cast<int>(state.last_transition_to_elevation);
  return stream.str();
}

/**
 * @brief Returns level 3D fall event to string.
 */
std::string Level3DFallEventToString(const Level3DPlayerState& state) {
  std::ostringstream stream;
  stream << "fall_land from=" << state.last_fall_from_tile_x << ','
         << state.last_fall_from_tile_y << " to=" << state.last_fall_to_tile_x
         << ',' << state.last_fall_to_tile_y
         << " drop=" << state.last_fall_drop_levels
         << " damage=" << state.last_fall_damage
         << " hp=" << state.current_hp << '/' << state.max_hp;
  return stream.str();
}

/**
 * @brief Returns level 3D health event to string.
 */
std::string Level3DHealthEventToString(const Level3DPlayerState& state) {
  std::ostringstream stream;
  stream << "hp_change damage=" << state.last_health_damage
         << " hp=" << state.last_health_before_hp << "->"
         << state.last_health_after_hp << '/' << state.max_hp;
  if (state.last_health_after_hp <= 0) {
    stream << " status=down";
  }
  return stream.str();
}

/**
 * @brief Returns visibility breakdown to string.
 */
std::string Level3DVisibilityBreakdownToString(
    const Level3DVisibilityBreakdown& breakdown) {
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(2);
  stream << "vis_breakdown="
         << "post=" << breakdown.posture_factor
         << " ter=" << breakdown.terrain_factor
         << " con=" << breakdown.concealment_factor
         << " cov=" << breakdown.cover_factor
         << " los=" << breakdown.vision_factor
         << " elev=" << breakdown.elevation_factor
         << " mov=" << breakdown.movement_factor
         << " raw=" << breakdown.raw_score
         << " final=" << breakdown.final_score;
  return stream.str();
}

/**
 * @brief Returns level 3D player tile diagnostics to string.
 */
std::string Level3DPlayerTileDiagnosticsToString(
    const Level3DPlayerTileDiagnostics& diagnostics) {
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(2);
  stream << "tile=" << std::setw(3) << diagnostics.tile_x << ','
         << std::setw(3) << diagnostics.tile_y
         << " ter=" << TerrainShortName(diagnostics.terrain)
         << " el=" << static_cast<int>(diagnostics.elevation)
         << " w=" << (diagnostics.walkable ? 'Y' : 'N')
         << " col=" << (diagnostics.collision ? 'Y' : 'N')
         << " con=" << static_cast<int>(diagnostics.concealment)
         << " posture=" << Level3DPlayerPostureName(diagnostics.posture)
         << " vis=" << diagnostics.visibility_score
         << " vpost=" << diagnostics.visibility.posture_factor
         << " vter=" << diagnostics.visibility.terrain_factor
         << " vcon=" << diagnostics.visibility.concealment_factor
         << " vcov=" << diagnostics.visibility.cover_factor
         << " velev=" << diagnostics.visibility.elevation_factor
         << " vmov=" << diagnostics.visibility.movement_factor
         << " mov=" << diagnostics.movement_multiplier
         << " post_mul=" << diagnostics.posture_speed_multiplier
         << " bs=" << diagnostics.base_speed_tiles_per_sec
         << " es=" << diagnostics.effective_speed_tiles_per_sec
         << " vel=" << diagnostics.velocity_x_tiles_per_sec << ','
         << diagnostics.velocity_y_tiles_per_sec
         << " face=" << diagnostics.facing_x << ',' << diagnostics.facing_y;
  return stream.str();
}

/**
 * @brief Returns level 3D player state to string.
 */
std::string Level3DPlayerStateToString(const Level3DPlayerState& state) {
  std::ostringstream stream;
  stream << "player3d: tile=" << state.tile_x << ',' << state.tile_y
         << " elevation=" << static_cast<int>(state.elevation)
         << " hp=" << state.current_hp << '/' << state.max_hp
         << " posture=" << Level3DPlayerPostureName(state.posture)
         << " visibility=" << state.visibility_score
         << " facing=" << state.facing_x << ',' << state.facing_y
         << " velocity=" << state.velocity_x_tiles_per_sec << ','
         << state.velocity_y_tiles_per_sec
         << " movement_multiplier=" << state.current_movement_multiplier
         << " posture_speed_multiplier=" << PostureSpeedMultiplier(state)
         << " target_movement_multiplier=" << state.target_movement_multiplier
         << " base_speed=" << state.move_speed_tiles_per_sec
         << " effective_speed=" << state.effective_move_speed_tiles_per_sec
         << " jump_active=" << (state.jump_active ? "true" : "false")
         << " jump_kind=" << Level3DJumpKindName(state.jump_kind);
  return stream.str();
}

}  // namespace sar::render3d
