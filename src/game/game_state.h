#ifndef SHOOT_AND_RUN_CPP_SRC_GAME_GAME_STATE_H_
#define SHOOT_AND_RUN_CPP_SRC_GAME_GAME_STATE_H_

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
