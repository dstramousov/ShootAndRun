#include "render3d/level_3d_camera.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace sar::render3d {
namespace {

constexpr float kPi = 3.14159265358979323846F;
constexpr float kTargetEyeHeight = 0.35F;
constexpr float kVectorEpsilon = 0.0001F;

float RadiansToDegrees(float radians) {
  return radians * 180.0F / kPi;
}

float ExponentialAlpha(float speed, float dt) {
  if (speed <= 0.0F || dt <= 0.0F) {
    return 0.0F;
  }
  return std::clamp(1.0F - std::exp(-speed * dt), 0.0F, 1.0F);
}

float SmoothFloat(float current, float target, float speed, float dt) {
  return current + (target - current) * ExponentialAlpha(speed, dt);
}

Vector3 AddVector3(Vector3 lhs, Vector3 rhs) {
  return Vector3{lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

Vector3 SubtractVector3(Vector3 lhs, Vector3 rhs) {
  return Vector3{lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

Vector3 ScaleVector3(Vector3 value, float scale) {
  return Vector3{value.x * scale, value.y * scale, value.z * scale};
}

float LengthSquaredXZ(Vector3 value) {
  return value.x * value.x + value.z * value.z;
}

Vector3 NormalizeXZOrFallback(Vector3 value, Vector3 fallback) {
  const float length_squared = LengthSquaredXZ(value);
  if (length_squared <= kVectorEpsilon) {
    return fallback;
  }
  const float inv_length = 1.0F / std::sqrt(length_squared);
  return Vector3{value.x * inv_length, 0.0F, value.z * inv_length};
}

Vector3 SmoothVector3(Vector3 current, Vector3 target, float speed, float dt) {
  const float alpha = ExponentialAlpha(speed, dt);
  return Vector3{current.x + (target.x - current.x) * alpha,
                 current.y + (target.y - current.y) * alpha,
                 current.z + (target.z - current.z) * alpha};
}

Vector3 PlayerTargetPosition(const LevelData& level,
                             const Level3DPlayerState& player,
                             float tile_world_size,
                             float elevation_step) {
  Vector3 position = Level3DPlayerWorldPosition(level, player, tile_world_size,
                                                elevation_step);
  position.y += kTargetEyeHeight;
  return position;
}

Vector3 PlayerFacingForward(const Level3DPlayerState& player) {
  return NormalizeXZOrFallback(
      Vector3{player.facing_x, 0.0F, player.facing_y},
      Vector3{0.0F, 0.0F, -1.0F});
}

Vector3 ClampXZToMapBounds(const LevelData& level,
                           Vector3 value,
                           float tile_world_size,
                           const Level3DCameraState& state) {
  const float half_width = static_cast<float>(level.size.width) *
                           tile_world_size * 0.5F;
  const float half_height = static_cast<float>(level.size.height) *
                            tile_world_size * 0.5F;
  const float min_margin = state.min_bounds_margin_tiles * tile_world_size;
  const float max_margin = state.max_bounds_margin_tiles * tile_world_size;
  const float margin = std::clamp(state.distance * state.bounds_margin_factor,
                                  min_margin, max_margin);

  if (half_width * 2.0F <= margin * 2.0F) {
    value.x = 0.0F;
  } else {
    value.x = std::clamp(value.x, -half_width + margin,
                         half_width - margin);
  }

  if (half_height * 2.0F <= margin * 2.0F) {
    value.z = 0.0F;
  } else {
    value.z = std::clamp(value.z, -half_height + margin,
                         half_height - margin);
  }

  return value;
}

Vector3 BuildDesiredCameraPosition(Vector3 anchor,
                                   Vector3 forward,
                                   const Level3DCameraState& state) {
  Vector3 position = SubtractVector3(anchor, ScaleVector3(forward, state.distance));
  position.y = anchor.y + state.height;
  return position;
}

}  // namespace

void InitializeLevel3DCamera(const LevelData& level,
                             Level3DCameraState* state) {
  if (state == nullptr) {
    return;
  }

  const float largest_side = static_cast<float>(
      std::max(level.size.width, level.size.height));
  state->distance = std::clamp(largest_side * 0.32F, state->min_distance,
                               state->max_distance);
  state->distance_target = state->distance;
  state->height = std::clamp(state->distance * 0.42F, 10.0F, 26.0F);
  state->yaw_deg = 270.0F;
  state->lookahead = Vector3{0.0F, 0.0F, 0.0F};
  state->last_forward = Vector3{0.0F, 0.0F, -1.0F};
  state->position = Vector3{0.0F, state->height, state->distance};
  state->target = Vector3{0.0F, 0.0F, 0.0F};
  state->initialized = false;
}

void UpdateLevel3DCamera(const LevelData& level,
                         const Level3DPlayerState& player,
                         const InputState& input,
                         float dt,
                         float tile_world_size,
                         float elevation_step,
                         Level3DCameraState* state) {
  if (state == nullptr || level.size.width <= 0 || level.size.height <= 0) {
    return;
  }

  const float safe_dt = std::clamp(dt, 0.0F, 0.05F);

  if (input.mouse_wheel_delta != 0.0F) {
    state->distance_target -= input.mouse_wheel_delta * state->zoom_step;
  }
  state->distance_target = std::clamp(state->distance_target,
                                      state->min_distance,
                                      state->max_distance);
  state->distance = SmoothFloat(state->distance, state->distance_target,
                                state->zoom_smooth_speed, safe_dt);
  state->height = std::clamp(state->distance * 0.42F, 10.0F, 26.0F);

  const Vector3 desired_forward = PlayerFacingForward(player);
  state->last_forward = SmoothVector3(state->last_forward, desired_forward,
                                      state->forward_smooth_speed, safe_dt);
  state->last_forward = NormalizeXZOrFallback(state->last_forward,
                                              desired_forward);

  const Vector3 desired_lookahead = ScaleVector3(
      state->last_forward,
      state->movement_lookahead_tiles * tile_world_size);
  state->lookahead = SmoothVector3(state->lookahead, desired_lookahead,
                                   state->lookahead_smooth_speed, safe_dt);

  const Vector3 player_target = PlayerTargetPosition(level, player,
                                                     tile_world_size,
                                                     elevation_step);
  Vector3 anchor = AddVector3(player_target, state->lookahead);
  anchor = ClampXZToMapBounds(level, anchor, tile_world_size, *state);

  Vector3 desired_target = AddVector3(
      anchor, ScaleVector3(state->last_forward,
                           state->target_lookahead_tiles * tile_world_size));
  desired_target = ClampXZToMapBounds(level, desired_target, tile_world_size,
                                      *state);

  const Vector3 desired_position = BuildDesiredCameraPosition(
      anchor, state->last_forward, *state);

  if (!state->initialized) {
    state->target = desired_target;
    state->position = desired_position;
    state->initialized = true;
  } else {
    state->target = SmoothVector3(state->target, desired_target,
                                  state->follow_smooth_speed, safe_dt);
    state->position = SmoothVector3(state->position, desired_position,
                                    state->follow_smooth_speed, safe_dt);
  }

  state->yaw_deg = RadiansToDegrees(std::atan2(state->last_forward.z,
                                               state->last_forward.x));
}

Camera3D BuildLevel3DCamera(const LevelData& level,
                            const Level3DPlayerState& player,
                            const Level3DCameraState& state,
                            const WindowState& window,
                            float tile_world_size,
                            float elevation_step) {
  const Vector3 fallback_target = PlayerTargetPosition(level, player,
                                                       tile_world_size,
                                                       elevation_step);

  Camera3D camera{};
  camera.position = state.initialized ? state.position
                                      : Vector3{fallback_target.x,
                                                fallback_target.y + state.height,
                                                fallback_target.z + state.distance};
  camera.target = state.initialized ? state.target : fallback_target;
  camera.up = Vector3{0.0F, 1.0F, 0.0F};
  camera.fovy = window.width > window.height ? 45.0F : 52.0F;
  camera.projection = CAMERA_PERSPECTIVE;
  return camera;
}

std::string Level3DCameraStateToString(const Level3DCameraState& state) {
  std::ostringstream stream;
  stream << "camera3d: yaw=" << state.yaw_deg
         << " distance=" << state.distance
         << "/" << state.distance_target
         << " height=" << state.height
         << " target=" << state.target.x << ',' << state.target.y << ','
         << state.target.z
         << " position=" << state.position.x << ',' << state.position.y << ','
         << state.position.z;
  return stream.str();
}

}  // namespace sar::render3d
