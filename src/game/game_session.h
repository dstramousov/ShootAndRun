#ifndef SHOOT_AND_RUN_CPP_SRC_GAME_GAME_SESSION_H_
#define SHOOT_AND_RUN_CPP_SRC_GAME_GAME_SESSION_H_

#include "game/game_state.h"

namespace sar {

class GameSession {
 public:
  /**
   * @brief Creates an empty game session.
   */
  GameSession() = default;

  /**
   * @brief Starts a new runtime session.
   */
  void StartNewGame();

  /**
   * @brief Returns current game session state.
   *
   * @return Current game state.
   */
  GameState state() const;

  /**
   * @brief Checks whether a playable game session exists.
   *
   * @return true if a game session is active.
   */
  bool is_active() const;

 private:
  GameState state_ = GameState::kNotStarted;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_GAME_GAME_SESSION_H_
