#include "render3d/level_3d_player_controller.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <sstream>
#include <string_view>

namespace sar::render3d {
namespace {

constexpr float kVectorEpsilon = 0.0001F;

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

bool CanEnterTile(const LevelData& level, const Level3DPlayerState& state,
                  float next_x, float next_y) {
  const int x = TileIndexFromPosition(next_x);
  const int y = TileIndexFromPosition(next_y);
  const RuntimeCell* target = CellAt(level, x, y);
  if (target == nullptr) {
    return false;
  }
  if (target->collision || !target->walkable) {
    return false;
  }
  if (target->height < 0 && state.elevation >= 0) {
    return false;
  }

  const int height_delta = std::abs(static_cast<int>(target->height) -
                                    static_cast<int>(state.elevation));
  return height_delta <= state.allowed_step_height;
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
  if (!CanEnterTile(level, *state, next_x, next_y)) {
    return;
  }

  state->tile_x = next_x;
  state->tile_y = next_y;
  state->elevation = HeightAtOrZero(level, TileIndexFromPosition(state->tile_x),
                                    TileIndexFromPosition(state->tile_y));
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

void UpdateVelocityFromInput(const InputState& input, float safe_dt,
                             Level3DPlayerState* state) {
  if (state == nullptr) {
    return;
  }

  float direction_x = 0.0F;
  float direction_y = 0.0F;
  FacingRelativeInputDirection(input, *state, &direction_x, &direction_y);

  const float target_x = direction_x * state->move_speed_tiles_per_sec;
  const float target_y = direction_y * state->move_speed_tiles_per_sec;
  const float acceleration = (direction_x == 0.0F && direction_y == 0.0F)
                                 ? state->deceleration_tiles_per_sec2
                                 : state->acceleration_tiles_per_sec2;
  MoveVelocityToward(target_x, target_y, acceleration * safe_dt, state);
}

}  // namespace

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
  UpdateVelocityFromInput(input, safe_dt, state);

  const float dx = state->velocity_x_tiles_per_sec * safe_dt;
  const float dy = state->velocity_y_tiles_per_sec * safe_dt;
  if (std::abs(dx) <= kVectorEpsilon && std::abs(dy) <= kVectorEpsilon) {
    return;
  }

  if (CanEnterTile(level, *state, state->tile_x + dx, state->tile_y + dy)) {
    ApplyMovementAxis(level, dx, dy, state);
    return;
  }

  ApplyMovementAxis(level, dx, 0.0F, state);
  ApplyMovementAxis(level, 0.0F, dy, state);
}

Vector3 Level3DPlayerWorldPosition(const LevelData& level,
                                   const Level3DPlayerState& state,
                                   float tile_world_size,
                                   float elevation_step) {
  const float origin_x = static_cast<float>(level.size.width) * tile_world_size *
                         0.5F;
  const float origin_z = static_cast<float>(level.size.height) *
                         tile_world_size * 0.5F;
  return Vector3{state.tile_x * tile_world_size - origin_x,
                 static_cast<float>(state.elevation) * elevation_step,
                 state.tile_y * tile_world_size - origin_z};
}

std::string Level3DPlayerStateToString(const Level3DPlayerState& state) {
  std::ostringstream stream;
  stream << "player3d: tile=" << state.tile_x << ',' << state.tile_y
         << " elevation=" << static_cast<int>(state.elevation)
         << " facing=" << state.facing_x << ',' << state.facing_y
         << " velocity=" << state.velocity_x_tiles_per_sec << ','
         << state.velocity_y_tiles_per_sec;
  return stream.str();
}

}  // namespace sar::render3d
