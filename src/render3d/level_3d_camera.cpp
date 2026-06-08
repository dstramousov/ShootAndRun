#include "render3d/level_3d_camera.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace sar::render3d {
namespace {

constexpr float kPi = 3.14159265358979323846F;
constexpr float kTargetEyeHeight = 0.35F;
constexpr float kVectorEpsilon = 0.0001F;

float DegreesToRadians(float degrees) {
  return degrees * kPi / 180.0F;
}

float ExponentialAlpha(float speed, float dt) {
  if (speed <= 0.0F || dt <= 0.0F) {
    return 0.0F;
  }
  return std::clamp(1.0F - std::exp(-speed * dt), 0.0F, 1.0F);
}

float WrapDegrees(float degrees) {
  float wrapped = std::fmod(degrees, 360.0F);
  if (wrapped < 0.0F) {
    wrapped += 360.0F;
  }
  return wrapped;
}

float ShortestAngleDelta(float from_deg, float to_deg) {
  float delta = std::fmod(to_deg - from_deg + 540.0F, 360.0F) - 180.0F;
  if (delta < -180.0F) {
    delta += 360.0F;
  }
  return delta;
}

float SmoothAngle(float current_deg, float target_deg, float speed, float dt) {
  const float delta = ShortestAngleDelta(current_deg, target_deg);
  return WrapDegrees(current_deg + delta * ExponentialAlpha(speed, dt));
}

float SmoothFloat(float current, float target, float speed, float dt) {
  return current + (target - current) * ExponentialAlpha(speed, dt);
}

Vector3 AddVector3(Vector3 lhs, Vector3 rhs) {
  return Vector3{lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}


Vector3 ScaleVector3(Vector3 value, float scale) {
  return Vector3{value.x * scale, value.y * scale, value.z * scale};
}

float LengthSquaredXZ(Vector3 value) {
  return value.x * value.x + value.z * value.z;
}

Vector3 NormalizeXZOrZero(Vector3 value) {
  const float length_squared = LengthSquaredXZ(value);
  if (length_squared <= kVectorEpsilon) {
    return Vector3{0.0F, 0.0F, 0.0F};
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

Vector3 MovementForwardFromInput(const InputState& input) {
  Vector3 direction{0.0F, 0.0F, 0.0F};
  if (input.left_down) {
    direction.x -= 1.0F;
  }
  if (input.right_down) {
    direction.x += 1.0F;
  }
  if (input.up_down) {
    direction.z -= 1.0F;
  }
  if (input.down_down) {
    direction.z += 1.0F;
  }
  return NormalizeXZOrZero(direction);
}

Vector3 ClampTargetToMapBounds(const LevelData& level,
                               Vector3 target,
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
    target.x = 0.0F;
  } else {
    target.x = std::clamp(target.x, -half_width + margin,
                          half_width - margin);
  }

  if (half_height * 2.0F <= margin * 2.0F) {
    target.z = 0.0F;
  } else {
    target.z = std::clamp(target.z, -half_height + margin,
                          half_height - margin);
  }

  return target;
}

void UpdateYawPitchTargetsFromInput(const InputState& input,
                                    float safe_dt,
                                    Level3DCameraState* state) {
  if (state == nullptr) {
    return;
  }

  if (input.camera_rotate_left_down) {
    state->yaw_target_deg -= state->rotate_speed_deg_per_sec * safe_dt;
  }
  if (input.camera_rotate_right_down) {
    state->yaw_target_deg += state->rotate_speed_deg_per_sec * safe_dt;
  }

  state->mouse_look_active = input.right_mouse_down;
  if (input.right_mouse_down) {
    state->yaw_target_deg += input.mouse_delta.x * state->mouse_yaw_sensitivity;
    state->pitch_target_deg -= input.mouse_delta.y *
                               state->mouse_pitch_sensitivity;
  }

  state->yaw_target_deg = WrapDegrees(state->yaw_target_deg);
  state->pitch_target_deg = std::clamp(state->pitch_target_deg,
                                       state->min_pitch_deg,
                                       state->max_pitch_deg);
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
  state->yaw_deg = 45.0F;
  state->yaw_target_deg = state->yaw_deg;
  state->pitch_deg = 55.0F;
  state->pitch_target_deg = state->pitch_deg;
  state->lookahead = Vector3{0.0F, 0.0F, 0.0F};
  state->last_forward = Vector3{0.0F, 0.0F, -1.0F};
  state->target = Vector3{0.0F, 0.0F, 0.0F};
  state->target_initialized = false;
  state->mouse_look_active = false;
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
  UpdateYawPitchTargetsFromInput(input, safe_dt, state);

  if (input.mouse_wheel_delta != 0.0F) {
    state->distance_target -= input.mouse_wheel_delta * state->zoom_step;
  }
  state->distance_target = std::clamp(state->distance_target,
                                      state->min_distance,
                                      state->max_distance);

  Vector3 desired_forward = MovementForwardFromInput(input);
  if (LengthSquaredXZ(desired_forward) > kVectorEpsilon) {
    state->last_forward = SmoothVector3(state->last_forward, desired_forward,
                                        state->forward_smooth_speed, safe_dt);
    state->last_forward = NormalizeXZOrZero(state->last_forward);
  }

  const Vector3 desired_lookahead = ScaleVector3(
      state->last_forward,
      state->lookahead_distance_tiles * tile_world_size);
  state->lookahead = SmoothVector3(state->lookahead, desired_lookahead,
                                   state->lookahead_smooth_speed, safe_dt);

  const Vector3 player_target = PlayerTargetPosition(level, player,
                                                     tile_world_size,
                                                     elevation_step);
  const Vector3 raw_target = AddVector3(player_target, state->lookahead);
  const Vector3 clamped_target = ClampTargetToMapBounds(
      level, raw_target, tile_world_size, *state);

  if (!state->target_initialized) {
    state->target = clamped_target;
    state->target_initialized = true;
  } else {
    state->target = SmoothVector3(state->target, clamped_target,
                                  state->follow_smooth_speed, safe_dt);
  }

  state->yaw_deg = SmoothAngle(state->yaw_deg, state->yaw_target_deg,
                               state->yaw_smooth_speed, safe_dt);
  state->pitch_deg = SmoothFloat(state->pitch_deg, state->pitch_target_deg,
                                 state->pitch_smooth_speed, safe_dt);
  state->distance = SmoothFloat(state->distance, state->distance_target,
                                state->zoom_smooth_speed, safe_dt);
}

Camera3D BuildLevel3DCamera(const LevelData& level,
                            const Level3DPlayerState& player,
                            const Level3DCameraState& state,
                            const WindowState& window,
                            float tile_world_size,
                            float elevation_step) {
  const Vector3 target = state.target_initialized
                             ? state.target
                             : PlayerTargetPosition(level, player,
                                                    tile_world_size,
                                                    elevation_step);
  const float yaw = DegreesToRadians(state.yaw_deg);
  const float pitch = DegreesToRadians(state.pitch_deg);
  const float horizontal_distance = std::cos(pitch) * state.distance;
  const float vertical_distance = std::sin(pitch) * state.distance;

  Camera3D camera{};
  camera.position = Vector3{target.x + std::cos(yaw) * horizontal_distance,
                            target.y + vertical_distance,
                            target.z + std::sin(yaw) * horizontal_distance};
  camera.target = target;
  camera.up = Vector3{0.0F, 1.0F, 0.0F};
  camera.fovy = window.width > window.height ? 45.0F : 52.0F;
  camera.projection = CAMERA_PERSPECTIVE;
  return camera;
}

std::string Level3DCameraStateToString(const Level3DCameraState& state) {
  std::ostringstream stream;
  stream << "camera3d: yaw=" << state.yaw_deg
         << "/" << state.yaw_target_deg
         << " pitch=" << state.pitch_deg
         << "/" << state.pitch_target_deg
         << " distance=" << state.distance
         << "/" << state.distance_target
         << " target=" << state.target.x << ',' << state.target.y << ','
         << state.target.z
         << " mouse=" << (state.mouse_look_active ? "on" : "off");
  return stream.str();
}

}  // namespace sar::render3d
