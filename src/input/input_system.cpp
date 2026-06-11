/**
 * @file src/input/input_system.cpp
 * @brief Normalized frame input snapshots and polling. Contains implementation for
 * input_system.cpp.
 */

#include "input/input_system.h"

#include <raylib.h>

namespace sar {

/**
 * @brief Implements InputSystem::Poll.
 */
InputState InputSystem::Poll() {
  InputState input;
  input.up_pressed = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
  input.down_pressed = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
  input.left_pressed = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A);
  input.right_pressed = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
  input.up_down = IsKeyDown(KEY_UP) || IsKeyDown(KEY_W);
  input.down_down = IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S);
  input.left_down = IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A);
  input.right_down = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
  input.confirm_pressed = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
  const bool escape_down = IsKeyDown(KEY_ESCAPE);
  input.cancel_pressed = IsKeyPressed(KEY_ESCAPE) ||
                         (escape_down && !escape_was_down_);
  input.cancel_down = escape_down;
  escape_was_down_ = escape_down;
  input.left_mouse_pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
  input.jump_pressed = IsKeyPressed(KEY_SPACE);
  input.crouch_pressed = IsKeyPressed(KEY_C);
  input.prone_pressed = IsKeyPressed(KEY_Z);
  input.run_down = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
  input.interact_pressed = IsKeyPressed(KEY_E);
  input.debug_view_raw_pressed = IsKeyPressed(KEY_F1);
  input.debug_view_analysis_pressed = IsKeyPressed(KEY_F2);
  input.debug_view_visual_pressed = IsKeyPressed(KEY_F3);
  input.debug_view_final_render_pressed = IsKeyPressed(KEY_F4);
  input.debug_elevation_overlay_pressed = IsKeyPressed(KEY_F6);
  input.debug_elevation_logs_pressed = IsKeyPressed(KEY_F7);
  input.debug_render3d_perf_pressed = IsKeyPressed(KEY_F8);
  input.mouse_wheel_delta = GetMouseWheelMove();
  input.mouse_position.x = static_cast<float>(GetMouseX());
  input.mouse_position.y = static_cast<float>(GetMouseY());
  const Vector2 mouse_delta = GetMouseDelta();
  input.mouse_delta.x = mouse_delta.x;
  input.mouse_delta.y = mouse_delta.y;
  return input;
}

}  // namespace sar
