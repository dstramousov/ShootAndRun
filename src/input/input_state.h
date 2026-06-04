#ifndef SHOOT_AND_RUN_CPP_SRC_INPUT_INPUT_STATE_H_
#define SHOOT_AND_RUN_CPP_SRC_INPUT_INPUT_STATE_H_

#include "core/types.h"

namespace sar {

struct InputState {
  bool up_pressed = false;
  bool down_pressed = false;
  bool left_pressed = false;
  bool right_pressed = false;
  bool confirm_pressed = false;
  bool cancel_pressed = false;
  bool cancel_down = false;
  bool left_mouse_pressed = false;
  Vec2 mouse_position;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_INPUT_INPUT_STATE_H_
