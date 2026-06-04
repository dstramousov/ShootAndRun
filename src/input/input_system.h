#ifndef SHOOT_AND_RUN_CPP_SRC_INPUT_INPUT_SYSTEM_H_
#define SHOOT_AND_RUN_CPP_SRC_INPUT_INPUT_SYSTEM_H_

#include "input/input_state.h"

namespace sar {

class InputSystem {
 public:
  /**
   * @brief Polls raylib input and returns normalized input state.
   *
   * @return Normalized input state for the current frame.
   */
  InputState Poll();

 private:
  bool escape_was_down_ = false;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_INPUT_INPUT_SYSTEM_H_
