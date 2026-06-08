#ifndef SHOOT_AND_RUN_CPP_SRC_UI_MENU_ITEM_H_
#define SHOOT_AND_RUN_CPP_SRC_UI_MENU_ITEM_H_

/**
 * @file src/ui/menu_item.h
 * @brief Main menu, confirmation dialog, and UI layout. Contains public declarations for
 * menu_item.h.
 */

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
  std::string id;  ///< Stable identifier loaded from source data or configuration.
  std::string title;  ///< Title value carried by this data structure.
  MenuAction action = MenuAction::kNewGame;  ///< Action value carried by this data structure.
  bool enabled = true;  ///< true when this configuration block or feature is enabled.
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_UI_MENU_ITEM_H_
