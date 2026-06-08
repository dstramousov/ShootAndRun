#ifndef SHOOT_AND_RUN_CPP_SRC_UI_MENU_ITEM_H_
#define SHOOT_AND_RUN_CPP_SRC_UI_MENU_ITEM_H_

#include <string>

namespace sar {

/**
 * @brief Action executed when a main-menu item is activated.
 */
enum class MenuAction {
  kNewGame,
  kLoadGame,
  kSaveGame,
  kSettings,
  kExit,
};

/**
 * @brief Immutable menu item data used by menu state and renderers.
 */
struct MenuItem {
  std::string id;
  std::string title;
  MenuAction action = MenuAction::kNewGame;
  bool enabled = true;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_UI_MENU_ITEM_H_
