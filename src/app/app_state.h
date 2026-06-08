#ifndef SHOOT_AND_RUN_CPP_SRC_APP_APP_STATE_H_
#define SHOOT_AND_RUN_CPP_SRC_APP_APP_STATE_H_

namespace sar {

/**
 * @brief High-level application screen currently receiving input and drawing.
 */
enum class AppScreen {
  kMainMenu,
  kMapPreparing,
  kGame,
  kSettings,
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_APP_APP_STATE_H_
