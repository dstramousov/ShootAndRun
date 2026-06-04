#include "game/game_session.h"

namespace sar {

void GameSession::StartNewGame() { state_ = GameState::kRunning; }

GameState GameSession::state() const { return state_; }

bool GameSession::is_active() const { return state_ != GameState::kNotStarted; }

}  // namespace sar
