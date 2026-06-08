/**
 * @file src/game/game_session.cpp
 * @brief Game session state and high-level game lifecycle. Contains implementation for
 * game_session.cpp.
 */

#include "game/game_session.h"

namespace sar {

void GameSession::StartNewGame() { state_ = GameState::kRunning; }

GameState GameSession::state() const { return state_; }

bool GameSession::is_active() const { return state_ != GameState::kNotStarted; }

}  // namespace sar
