#ifndef SHOOT_AND_RUN_CPP_SRC_INPUT_INPUT_STATE_H_
#define SHOOT_AND_RUN_CPP_SRC_INPUT_INPUT_STATE_H_

#include "core/types.h"

namespace sar {

/**
 * @brief Frame-local input snapshot used by gameplay, UI and render modules.
 *
 * The structure contains raw button transitions, held-key states and mouse
 * deltas collected once per frame by InputSystem. It intentionally stores only
 * plain values so it can be copied safely between update functions.
 */
struct InputState {
  bool up_pressed = false;
  bool down_pressed = false;
  bool left_pressed = false;
  bool right_pressed = false;
  bool up_down = false;
  bool down_down = false;
  bool left_down = false;
  bool right_down = false;
  bool confirm_pressed = false;
  bool cancel_pressed = false;
  bool cancel_down = false;
  bool left_mouse_pressed = false;
  bool jump_pressed = false;
  bool debug_view_raw_pressed = false;
  bool debug_view_analysis_pressed = false;
  bool debug_view_visual_pressed = false;
  bool debug_view_final_render_pressed = false;
  float mouse_wheel_delta = 0.0F;
  Vec2 mouse_position;
  Vec2 mouse_delta;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_INPUT_INPUT_STATE_H_
