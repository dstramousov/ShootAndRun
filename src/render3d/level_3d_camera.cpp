/**
 * @file src/render3d/level_3d_camera.cpp
 * @brief 3D renderer, camera, player movement, fog, and asset registry. Contains implementation
 * for level_3d_camera.cpp.
 */

#include "render3d/level_3d_camera.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace sar::render3d {
namespace {

constexpr float kPi = 3.14159265358979323846F;
constexpr float kTargetEyeHeight = 0.35F;
constexpr float kVectorEpsilon = 0.0001F;

/**
 * @brief Executes the radians to degrees operation.
 */
float RadiansToDegrees(float radians) {
  return radians * 180.0F / kPi;
}

/**
 * @brief Executes the exponential alpha operation.
 */
float ExponentialAlpha(float speed, float dt) {
  if (speed <= 0.0F || dt <= 0.0F) {
    return 0.0F;
  }
  return std::clamp(1.0F - std::exp(-speed * dt), 0.0F, 1.0F);
}

/**
 * @brief Executes the smooth float operation.
 */
float SmoothFloat(float current, float target, float speed, float dt) {
  return current + (target - current) * ExponentialAlpha(speed, dt);
}

/**
 * @brief Adds vector3.
 */
Vector3 AddVector3(Vector3 lhs, Vector3 rhs) {
  return Vector3{lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

/**
 * @brief Executes the subtract vector3 operation.
 */
Vector3 SubtractVector3(Vector3 lhs, Vector3 rhs) {
  return Vector3{lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

/**
 * @brief Executes the scale vector3 operation.
 */
Vector3 ScaleVector3(Vector3 value, float scale) {
  return Vector3{value.x * scale, value.y * scale, value.z * scale};
}

/**
 * @brief Executes the length squared xz operation.
 */
float LengthSquaredXZ(Vector3 value) {
  return value.x * value.x + value.z * value.z;
}

/**
 * @brief Executes the normalize xz or fallback operation.
 */
Vector3 NormalizeXZOrFallback(Vector3 value, Vector3 fallback) {
  const float length_squared = LengthSquaredXZ(value);
  if (length_squared <= kVectorEpsilon) {
    return fallback;
  }
  const float inv_length = 1.0F / std::sqrt(length_squared);
  return Vector3{value.x * inv_length, 0.0F, value.z * inv_length};
}

/**
 * @brief Executes the smooth vector3 operation.
 */
Vector3 SmoothVector3(Vector3 current, Vector3 target, float speed, float dt) {
  const float alpha = ExponentialAlpha(speed, dt);
  return Vector3{current.x + (target.x - current.x) * alpha,
                 current.y + (target.y - current.y) * alpha,
                 current.z + (target.z - current.z) * alpha};
}

/**
 * @brief Executes the lerp vector3 operation.
 */
Vector3 LerpVector3(Vector3 current, Vector3 target, float alpha) {
  return Vector3{current.x + (target.x - current.x) * alpha,
                 current.y + (target.y - current.y) * alpha,
                 current.z + (target.z - current.z) * alpha};
}

/**
 * @brief Executes the smooth step operation.
 */
float SmoothStep(float value) {
  const float t = std::clamp(value, 0.0F, 1.0F);
  return t * t * (3.0F - 2.0F * t);
}

/**
 * @brief Executes the rotate xz operation.
 */
Vector3 RotateXZ(Vector3 value, float degrees) {
  const float radians = degrees * kPi / 180.0F;
  const float sine = std::sin(radians);
  const float cosine = std::cos(radians);
  return Vector3{value.x * cosine - value.z * sine,
                 value.y,
                 value.x * sine + value.z * cosine};
}

/**
 * @brief Executes the player target position operation.
 */
Vector3 PlayerTargetPosition(const LevelData& level,
                             const Level3DPlayerState& player,
                             float tile_world_size,
                             float elevation_step) {
  Vector3 position = Level3DPlayerWorldPosition(level, player, tile_world_size,
                                                elevation_step);
  position.y += kTargetEyeHeight;
  return position;
}

/**
 * @brief Executes the player facing forward operation.
 */
Vector3 PlayerFacingForward(const Level3DPlayerState& player) {
  return NormalizeXZOrFallback(
      Vector3{player.facing_x, 0.0F, player.facing_y},
      Vector3{0.0F, 0.0F, -1.0F});
}

/**
 * @brief Clamps xz to map bounds to a safe range.
 */
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

/**
 * @brief Builds desired camera position.
 */
Vector3 BuildDesiredCameraPosition(Vector3 anchor,
                                   Vector3 forward,
                                   const Level3DCameraState& state) {
  Vector3 position = SubtractVector3(anchor, ScaleVector3(forward, state.distance));
  position.y = anchor.y + state.height;
  return position;
}

/**
 * @brief Stores camera pose 3D data shared between runtime systems.
 */
struct CameraPose3D {
  Vector3 position{0.0F, 0.0F, 0.0F};
  Vector3 target{0.0F, 0.0F, 0.0F};
  Vector3 lookahead{0.0F, 0.0F, 0.0F};
  Vector3 forward{0.0F, 0.0F, -1.0F};
};

/**
 * @brief Builds camera pose.
 */
CameraPose3D BuildCameraPose(const LevelData& level,
                             const Level3DPlayerState& player,
                             float tile_world_size,
                             float elevation_step,
                             float distance,
                             float height,
                             Vector3 forward,
                             const Level3DCameraState& state) {
  forward = NormalizeXZOrFallback(forward, PlayerFacingForward(player));
  const Vector3 lookahead = ScaleVector3(
      forward, state.movement_lookahead_tiles * tile_world_size);
  Vector3 anchor = AddVector3(
      PlayerTargetPosition(level, player, tile_world_size, elevation_step),
      lookahead);
  anchor = ClampXZToMapBounds(level, anchor, tile_world_size, state);

  Vector3 target = AddVector3(
      anchor, ScaleVector3(forward,
                           state.target_lookahead_tiles * tile_world_size));
  target = ClampXZToMapBounds(level, target, tile_world_size, state);

  Vector3 position = SubtractVector3(anchor, ScaleVector3(forward, distance));
  position.y = anchor.y + height;
  return CameraPose3D{position, target, lookahead, forward};
}

/**
 * @brief Executes the emit intro event operation.
 */
void EmitIntroEvent(Level3DCameraIntroEvent event,
                    Level3DCameraState* state) {
  if (state == nullptr) {
    return;
  }
  state->last_intro_event = event;
  ++state->intro_event_sequence;
}

/**
 * @brief Executes the finish intro operation.
 */
void FinishIntro(Level3DCameraIntroEvent event, Level3DCameraState* state) {
  if (state == nullptr) {
    return;
  }
  state->intro_active = false;
  state->intro_finished = true;
  state->intro_elapsed_sec = state->intro_duration_sec;
  state->distance = state->intro_end_distance;
  state->distance_target = state->intro_end_distance;
  state->height = state->intro_end_height;
  state->position = state->intro_end_position;
  state->target = state->intro_end_target;
  state->initialized = true;
  EmitIntroEvent(event, state);
}

}  // namespace

/**
 * @brief Initializes level 3D camera.
 */
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
  state->intro_active = false;
  state->intro_finished = false;
  state->intro_elapsed_sec = 0.0F;
  state->intro_start_position = Vector3{0.0F, 0.0F, 0.0F};
  state->intro_start_target = Vector3{0.0F, 0.0F, 0.0F};
  state->intro_end_position = Vector3{0.0F, 0.0F, 0.0F};
  state->intro_end_target = Vector3{0.0F, 0.0F, 0.0F};
  state->last_intro_event = Level3DCameraIntroEvent::kNone;
  state->intro_event_sequence = 0;
  state->initialized = false;
}

/**
 * @brief Starts level 3D camera intro.
 */
void StartLevel3DCameraIntro(const LevelData& level,
                             const Level3DPlayerState& player,
                             float tile_world_size,
                             float elevation_step,
                             Level3DCameraState* state) {
  if (state == nullptr || !state->intro_enabled) {
    return;
  }

  const Vector3 forward = PlayerFacingForward(player);
  const CameraPose3D end_pose = BuildCameraPose(
      level, player, tile_world_size, elevation_step,
      state->intro_end_distance, state->intro_end_height, forward, *state);
  const Vector3 start_forward = NormalizeXZOrFallback(
      RotateXZ(forward, state->intro_start_yaw_offset_deg), forward);
  const CameraPose3D start_pose = BuildCameraPose(
      level, player, tile_world_size, elevation_step,
      state->intro_start_distance, state->intro_start_height, start_forward,
      *state);

  state->intro_start_position = start_pose.position;
  state->intro_start_target = start_pose.target;
  state->intro_end_position = end_pose.position;
  state->intro_end_target = end_pose.target;
  state->intro_elapsed_sec = 0.0F;
  state->intro_active = true;
  state->intro_finished = false;
  state->distance = state->intro_start_distance;
  state->distance_target = state->intro_end_distance;
  state->height = state->intro_start_height;
  state->position = state->intro_start_position;
  state->target = state->intro_start_target;
  state->lookahead = end_pose.lookahead;
  state->last_forward = end_pose.forward;
  state->initialized = true;
  EmitIntroEvent(Level3DCameraIntroEvent::kStarted, state);
}

/**
 * @brief Checks whether level 3D camera intro active is true.
 */
bool IsLevel3DCameraIntroActive(const Level3DCameraState& state) {
  return state.intro_active;
}

/**
 * @brief Executes the level 3D camera intro locks player operation.
 */
bool Level3DCameraIntroLocksPlayer(const Level3DCameraState& state) {
  return state.intro_active && state.intro_lock_player_input;
}

/**
 * @brief Executes the level 3D camera intro skip requested operation.
 */
bool Level3DCameraIntroSkipRequested(const Level3DCameraState& state,
                                      const InputState& input) {
  return state.intro_active && state.intro_skip_enabled &&
         (input.confirm_pressed || input.jump_pressed ||
          input.left_mouse_pressed);
}

/**
 * @brief Executes the skip level 3D camera intro operation.
 */
void SkipLevel3DCameraIntro(Level3DCameraState* state) {
  if (state == nullptr || !state->intro_active) {
    return;
  }
  FinishIntro(Level3DCameraIntroEvent::kSkipped, state);
}

/**
 * @brief Updates level 3D camera for the current frame.
 */
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

  if (state->intro_active) {
    if (state->intro_duration_sec <= 0.0F) {
      FinishIntro(Level3DCameraIntroEvent::kFinished, state);
      return;
    }

    state->intro_elapsed_sec = std::min(
        state->intro_duration_sec, state->intro_elapsed_sec + safe_dt);
    const float progress = state->intro_elapsed_sec / state->intro_duration_sec;
    const float eased = SmoothStep(progress);
    state->distance = state->intro_start_distance +
                      (state->intro_end_distance - state->intro_start_distance) *
                          eased;
    state->distance_target = state->intro_end_distance;
    state->height = state->intro_start_height +
                    (state->intro_end_height - state->intro_start_height) * eased;
    state->position = LerpVector3(state->intro_start_position,
                                  state->intro_end_position, eased);
    state->target = LerpVector3(state->intro_start_target,
                                state->intro_end_target, eased);
    state->initialized = true;
    if (progress >= 1.0F) {
      FinishIntro(Level3DCameraIntroEvent::kFinished, state);
    }
    return;
  }

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

/**
 * @brief Builds level 3D camera.
 */
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

/**
 * @brief Returns level 3D camera intro event to string.
 */
std::string Level3DCameraIntroEventToString(
    const Level3DCameraState& state) {
  std::ostringstream stream;
  stream << "intro_";
  switch (state.last_intro_event) {
    case Level3DCameraIntroEvent::kStarted:
      stream << "start";
      break;
    case Level3DCameraIntroEvent::kSkipped:
      stream << "skip";
      break;
    case Level3DCameraIntroEvent::kFinished:
      stream << "finish";
      break;
    case Level3DCameraIntroEvent::kNone:
      stream << "none";
      break;
  }
  stream << " active=" << (state.intro_active ? "Y" : "N")
         << " elapsed=" << state.intro_elapsed_sec << "/"
         << state.intro_duration_sec
         << " distance=" << state.distance << "/" << state.distance_target
         << " height=" << state.height;
  return stream.str();
}

/**
 * @brief Returns level 3D camera state to string.
 */
std::string Level3DCameraStateToString(const Level3DCameraState& state) {
  std::ostringstream stream;
  stream << "camera3d: yaw=" << state.yaw_deg
         << " distance=" << state.distance
         << "/" << state.distance_target
         << " height=" << state.height
         << " intro=" << (state.intro_active ? "active" :
                            (state.intro_finished ? "done" : "off"))
         << " target=" << state.target.x << ',' << state.target.y << ','
         << state.target.z
         << " position=" << state.position.x << ',' << state.position.y << ','
         << state.position.z;
  return stream.str();
}

}  // namespace sar::render3d
