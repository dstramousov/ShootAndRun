#ifndef SHOOT_AND_RUN_CPP_SRC_INPUT_INPUT_SYSTEM_H_
#define SHOOT_AND_RUN_CPP_SRC_INPUT_INPUT_SYSTEM_H_

/**
 * @file src/input/input_system.h
 * @brief Normalized frame input snapshots and polling. Contains public declarations for
 * input_system.h.
 */

#include "input/input_state.h"

namespace sar {

/**
 * @brief Owns the input system behavior and its runtime state.
 */
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
