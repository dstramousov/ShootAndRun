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

bool TextContains(std::string_view text, std::string_view needle) {
  return text.find(needle) != std::string_view::npos;
}

bool IsPreferredSpawnMarker(const Marker& marker) {
  return marker.type == "player_spawn" || marker.id == "player_spawn" ||
         TextContains(marker.type, "player_spawn") ||
         TextContains(marker.id, "player_spawn");
}

bool IsFallbackSpawnMarker(const Marker& marker) {
  return marker.type == "start" || marker.id == "start" ||
         TextContains(marker.type, "spawn") || TextContains(marker.id, "spawn");
}

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

bool IsInsideMap(const LevelData& level, int x, int y) {
  return x >= 0 && y >= 0 && x < level.size.width && y < level.size.height;
}

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

std::int8_t HeightAtOrZero(const LevelData& level, int x, int y) {
  const RuntimeCell* cell = CellAt(level, x, y);
  return cell != nullptr ? cell->height : 0;
}

int TileIndexFromPosition(float value) {
  return static_cast<int>(std::floor(value));
}

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

float CurrentTileMovementMultiplier(const LevelData& level,
                                    const Level3DPlayerState& state) {
  const RuntimeCell* cell = CellAt(level, TileIndexFromPosition(state.tile_x),
                                   TileIndexFromPosition(state.tile_y));
  if (cell == nullptr || cell->collision || !cell->walkable) {
    return 0.0F;
  }
  return std::clamp(cell->movement_multiplier, 0.0F, 1.50F);
}

float MovementMultiplierForState(const LevelData& level,
                                 const Level3DPlayerState& state) {
  if (state.jump_active && state.jump_kind == Level3DJumpKind::kRun) {
    return std::clamp(state.jump_start_movement_multiplier, 0.0F, 1.50F);
  }
  return CurrentTileMovementMultiplier(level, state);
}

float ExponentialAlpha(float speed, float dt) {
  if (speed <= 0.0F || dt <= 0.0F) {
    return 1.0F;
  }
  return std::clamp(1.0F - std::exp(-speed * dt), 0.0F, 1.0F);
}

void RefreshEffectiveMovementSpeed(const LevelData& level,
                                   Level3DPlayerState* state) {
  if (state == nullptr) {
    return;
  }
  state->target_movement_multiplier = MovementMultiplierForState(level, *state);
  state->effective_move_speed_tiles_per_sec =
      state->move_speed_tiles_per_sec * state->current_movement_multiplier;
}

void ResetMovementMultiplier(const LevelData& level,
                             Level3DPlayerState* state) {
  if (state == nullptr) {
    return;
  }
  state->target_movement_multiplier = CurrentTileMovementMultiplier(
      level, *state);
  state->current_movement_multiplier = state->target_movement_multiplier;
  state->effective_move_speed_tiles_per_sec =
      state->move_speed_tiles_per_sec * state->current_movement_multiplier;
}

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
      state->move_speed_tiles_per_sec * state->current_movement_multiplier;
}

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

void FacingRelativeInputDirection(const InputState& input,
                                  const Level3DPlayerState& state,
                                  float* out_x,
                                  float* out_y);

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
      return from_height < 0 || to_height < 0;
    case ElevationTransitionType::kStep:
    case ElevationTransitionType::kUnknown:
      return false;
  }
  return false;
}

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

float CurrentHorizontalSpeed(const Level3DPlayerState& state) {
  return std::hypot(state.velocity_x_tiles_per_sec,
                    state.velocity_y_tiles_per_sec);
}

bool IsRunningJumpCandidate(const Level3DPlayerState& state) {
  return state.jump_active && state.jump_kind == Level3DJumpKind::kRun &&
         CurrentHorizontalSpeed(state) >= state.jump_min_running_speed_tiles_per_sec;
}

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
  if (transition != nullptr &&
      CanUseNormalMovementTransition(*transition, state.elevation,
                                     target->height)) {
    return {true, x, y, height_delta, Level3DMoveBlockReason::kNone,
            true, transition->type};
  }
  if ((target->height < 0 || state.elevation < 0) &&
      target->height != state.elevation) {
    return {false, x, y, height_delta, Level3DMoveBlockReason::kUnderground};
  }
  if (height_delta == 1 && IsRunningJumpCandidate(state)) {
    return {true, x, y, height_delta, Level3DMoveBlockReason::kNone,
            false, ElevationTransitionType::kUnknown, true};
  }
  if (height_delta > 0) {
    const Level3DMoveBlockReason reason =
        height_delta == 1 ? Level3DMoveBlockReason::kStepUpRequired
                          : Level3DMoveBlockReason::kHeightStep;
    return {false, x, y, height_delta, reason};
  }
  if (-height_delta > state.allowed_step_down_height) {
    return {false, x, y, height_delta, Level3DMoveBlockReason::kHeightStep};
  }

  return {true, x, y, height_delta, Level3DMoveBlockReason::kNone};
}

float Clamp01(float value) {
  return std::clamp(value, 0.0F, 1.0F);
}

float SmoothStep01(float value) {
  const float clamped = Clamp01(value);
  return clamped * clamped * (3.0F - 2.0F * clamped);
}

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
  if ((target->height < 0 || state.elevation < 0) &&
      target->height != state.elevation) {
    return {false, target_x, target_y, height_delta,
            Level3DMoveBlockReason::kUnderground};
  }
  if (transition != nullptr &&
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

bool TryStartRunningJump(const LevelData& level, const InputState& input,
                         Level3DPlayerState* state) {
  if (state == nullptr || !input.jump_pressed || state->jump_active ||
      state->step_jump_active) {
    return false;
  }

  RefreshEffectiveMovementSpeed(level, state);
  StartRunningJump(state);
  return true;
}

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

  state->elevation = HeightAtOrZero(level, landing_x, landing_y);
  state->step_jump_to_tile_x = landing_x;
  state->step_jump_to_tile_y = landing_y;
  state->step_jump_to_elevation = state->elevation;
  state->visual_elevation_offset = 0.0F;
  RecordJumpEvent(Level3DJumpEventType::kLanded,
                  Level3DMoveBlockReason::kNone, state);
  ResetJumpState(state);
  RefreshEffectiveMovementSpeed(level, state);
  return true;
}

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
  }
  RefreshEffectiveMovementSpeed(level, state);
  RecordTransitionEvent(enter_result, previous_state, state);
}

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
                      state->jump_horizontal_speed_multiplier;
    acceleration *= state->jump_air_control_multiplier;
  }

  const float target_x = direction_x * effective_speed;
  const float target_y = direction_y * effective_speed;
  MoveVelocityToward(target_x, target_y, acceleration * safe_dt, state);
}

}  // namespace

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
  }
  return "unknown";
}

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
  state->facing_x = 0.0F;
  state->facing_y = -1.0F;
  state->velocity_x_tiles_per_sec = 0.0F;
  state->velocity_y_tiles_per_sec = 0.0F;
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
  ResetMovementMultiplier(level, state);
  state->initialized = true;
}

void UpdateLevel3DPlayer(const LevelData& level, const InputState& input,
                         float dt, Level3DPlayerState* state) {
  if (state == nullptr || !state->initialized || level.size.width <= 0 ||
      level.size.height <= 0) {
    return;
  }

  const float safe_dt = std::clamp(dt, 0.0F, 0.05F);
  UpdateFacingFromMouse(input, state);
  if (UpdateStepJump(level, safe_dt, state)) {
    return;
  }
  const bool running_jump_active = UpdateRunningJump(level, safe_dt, state);
  if (TryStartStepJump(level, input, state)) {
    return;
  }
  if (!running_jump_active) {
    TryStartRunningJump(level, input, state);
  }
  UpdateVelocityFromInput(level, input, safe_dt, state);

  const float dx = state->velocity_x_tiles_per_sec * safe_dt;
  const float dy = state->velocity_y_tiles_per_sec * safe_dt;
  if (std::abs(dx) <= kVectorEpsilon && std::abs(dy) <= kVectorEpsilon) {
    return;
  }

  const EnterTileResult full_enter = CheckEnterTile(
      level, *state, state->tile_x + dx, state->tile_y + dy);
  if (full_enter.can_enter) {
    ApplyMovementAxis(level, dx, dy, state);
    return;
  }

  ApplyMovementAxis(level, dx, 0.0F, state);
  ApplyMovementAxis(level, 0.0F, dy, state);
  if (std::abs(dx) > kVectorEpsilon || std::abs(dy) > kVectorEpsilon) {
    RecordBlockedTile(full_enter.tile_x, full_enter.tile_y,
                      full_enter.reason, state);
  }
}

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

Level3DPlayerTileDiagnostics CurrentLevel3DPlayerTileDiagnostics(
    const LevelData& level,
    const Level3DPlayerState& state) {
  Level3DPlayerTileDiagnostics diagnostics;
  diagnostics.tile_x = TileIndexFromPosition(state.tile_x);
  diagnostics.tile_y = TileIndexFromPosition(state.tile_y);
  diagnostics.base_speed_tiles_per_sec = state.move_speed_tiles_per_sec;
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
         << " mov=" << diagnostics.movement_multiplier
         << " bs=" << diagnostics.base_speed_tiles_per_sec
         << " es=" << diagnostics.effective_speed_tiles_per_sec
         << " vel=" << diagnostics.velocity_x_tiles_per_sec << ','
         << diagnostics.velocity_y_tiles_per_sec
         << " face=" << diagnostics.facing_x << ',' << diagnostics.facing_y;
  return stream.str();
}

std::string Level3DPlayerStateToString(const Level3DPlayerState& state) {
  std::ostringstream stream;
  stream << "player3d: tile=" << state.tile_x << ',' << state.tile_y
         << " elevation=" << static_cast<int>(state.elevation)
         << " facing=" << state.facing_x << ',' << state.facing_y
         << " velocity=" << state.velocity_x_tiles_per_sec << ','
         << state.velocity_y_tiles_per_sec
         << " movement_multiplier=" << state.current_movement_multiplier
         << " target_movement_multiplier=" << state.target_movement_multiplier
         << " base_speed=" << state.move_speed_tiles_per_sec
         << " effective_speed=" << state.effective_move_speed_tiles_per_sec
         << " jump_active=" << (state.jump_active ? "true" : "false")
         << " jump_kind=" << Level3DJumpKindName(state.jump_kind);
  return stream.str();
}

}  // namespace sar::render3d
