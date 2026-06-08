#ifndef SHOOT_AND_RUN_CPP_SRC_GAME_GAME_STATE_H_
#define SHOOT_AND_RUN_CPP_SRC_GAME_GAME_STATE_H_

/**
 * @file src/game/game_state.h
 * @brief Game session state and high-level game lifecycle. Contains public declarations for
 * game_state.h.
 */

namespace sar {

/**
 * @brief Lifecycle state of the active game session.
 */
enum class GameState {
  kNotStarted,
  kRunning,
  kPaused,
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_GAME_GAME_STATE_H_
