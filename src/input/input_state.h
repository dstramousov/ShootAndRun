#ifndef SHOOT_AND_RUN_CPP_SRC_INPUT_INPUT_STATE_H_
#define SHOOT_AND_RUN_CPP_SRC_INPUT_INPUT_STATE_H_

/**
 * @file src/input/input_state.h
 * @brief Normalized frame input snapshots and polling. Contains public declarations for
 * input_state.h.
 */

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
  bool up_pressed = false;  ///< Up pressed value carried by this data structure.
  bool down_pressed = false;  ///< Down pressed value carried by this data structure.
  bool left_pressed = false;  ///< Left pressed value carried by this data structure.
  bool right_pressed = false;  ///< Right pressed value carried by this data structure.
  bool up_down = false;  ///< Up down value carried by this data structure.
  bool down_down = false;  ///< Down down value carried by this data structure.
  bool left_down = false;  ///< Left down value carried by this data structure.
  bool right_down = false;  ///< Right down value carried by this data structure.
  bool confirm_pressed = false;  ///< Confirm pressed value carried by this data structure.
  bool cancel_pressed = false;  ///< Cancel pressed value carried by this data structure.
  bool cancel_down = false;  ///< Cancel down value carried by this data structure.
  bool left_mouse_pressed = false;  ///< Left mouse pressed value carried by this data structure.
  bool jump_pressed = false;  ///< Jump pressed value carried by this data structure.
  bool crouch_pressed = false;  ///< true when the crouch posture toggle was pressed this frame.
  bool prone_pressed = false;  ///< true when the prone posture toggle was pressed this frame.
  bool debug_view_raw_pressed = false;  ///< Debug view raw pressed value carried by this data structure.
  bool debug_view_analysis_pressed = false;  ///< Debug view analysis pressed value carried by this data structure.
  bool debug_view_visual_pressed = false;  ///< Debug view visual pressed value carried by this data structure.
  bool debug_view_final_render_pressed = false;  ///< Debug view final render pressed value carried by this data structure.
  bool debug_elevation_overlay_pressed = false;  ///< Debug elevation overlay toggle pressed this frame.
  bool debug_elevation_logs_pressed = false;  ///< Debug elevation movement-log toggle pressed this frame.
  bool debug_render3d_perf_pressed = false;  ///< Debug 3D render performance overlay toggle pressed this frame.
  float mouse_wheel_delta = 0.0F;  ///< Mouse wheel delta value carried by this data structure.
  Vec2 mouse_position;  ///< Position value for mouse position.
  Vec2 mouse_delta;  ///< Mouse delta value carried by this data structure.
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_INPUT_INPUT_STATE_H_
