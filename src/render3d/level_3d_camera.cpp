#include "render3d/level_3d_camera.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace sar::render3d {
namespace {

constexpr float kPi = 3.14159265358979323846F;

float DegreesToRadians(float degrees) {
  return degrees * kPi / 180.0F;
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
  state->yaw_deg = 45.0F;
  state->pitch_deg = 55.0F;
}

void UpdateLevel3DCamera(const InputState& input, float dt,
                         Level3DCameraState* state) {
  if (state == nullptr) {
    return;
  }

  const float safe_dt = std::clamp(dt, 0.0F, 0.05F);
  if (input.camera_rotate_left_down) {
    state->yaw_deg -= state->rotate_speed_deg_per_sec * safe_dt;
  }
  if (input.camera_rotate_right_down) {
    state->yaw_deg += state->rotate_speed_deg_per_sec * safe_dt;
  }

  if (input.mouse_wheel_delta != 0.0F) {
    const float multiplier = std::pow(state->zoom_step,
                                      -input.mouse_wheel_delta);
    state->distance *= multiplier;
  }
  state->distance = std::clamp(state->distance, state->min_distance,
                               state->max_distance);
}

Camera3D BuildLevel3DCamera(const LevelData& level,
                            const Level3DPlayerState& player,
                            const Level3DCameraState& state,
                            const WindowState& window,
                            float tile_world_size,
                            float elevation_step) {
  const Vector3 target = Level3DPlayerWorldPosition(
      level, player, tile_world_size, elevation_step);
  const float yaw = DegreesToRadians(state.yaw_deg);
  const float pitch = DegreesToRadians(state.pitch_deg);
  const float horizontal_distance = std::cos(pitch) * state.distance;
  const float vertical_distance = std::sin(pitch) * state.distance;

  Camera3D camera{};
  camera.position = Vector3{target.x + std::cos(yaw) * horizontal_distance,
                            target.y + vertical_distance,
                            target.z + std::sin(yaw) * horizontal_distance};
  camera.target = Vector3{target.x, target.y + 0.35F, target.z};
  camera.up = Vector3{0.0F, 1.0F, 0.0F};
  camera.fovy = window.width > window.height ? 45.0F : 52.0F;
  camera.projection = CAMERA_PERSPECTIVE;
  return camera;
}

std::string Level3DCameraStateToString(const Level3DCameraState& state) {
  std::ostringstream stream;
  stream << "camera3d: yaw=" << state.yaw_deg
         << " distance=" << state.distance
         << " pitch=" << state.pitch_deg;
  return stream.str();
}

}  // namespace sar::render3d
