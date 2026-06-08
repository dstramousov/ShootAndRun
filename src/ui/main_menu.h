#ifndef SHOOT_AND_RUN_CPP_SRC_UI_MAIN_MENU_H_
#define SHOOT_AND_RUN_CPP_SRC_UI_MAIN_MENU_H_

/**
 * @file src/ui/main_menu.h
 * @brief Main menu, confirmation dialog, and UI layout. Contains public declarations for
 * main_menu.h.
 */

#include <string>
#include <vector>

#include "ui/menu_item.h"

namespace sar {

/**
 * @brief Runtime state and navigation logic for the main menu.
 */
class MainMenu {
 public:
  /**
   * @brief Creates the default main menu state.
   */
  MainMenu();

  /**
   * @brief Returns all menu items.
   *
   * @return Immutable menu item list.
   */
  const std::vector<MenuItem>& items() const;

  /**
   * @brief Returns the currently selected menu item index.
   *
   * @return Selected item index or -1 if no item is selectable.
   */
  int selected_index() const;

  /**
   * @brief Selects the next enabled menu item.
   */
  void SelectNext();

  /**
   * @brief Selects the previous enabled menu item.
   */
  void SelectPrevious();

  /**
   * @brief Selects an enabled menu item by index.
   *
   * @param index Candidate menu item index.
   * @return true if selection was changed.
   */
  bool SelectIndex(int index);

  /**
   * @brief Returns the currently selected item.
   *
   * @return Selected menu item or nullptr when no item is selected.
   */
  const MenuItem* SelectedItem() const;

  /**
   * @brief Activates the currently selected item.
   *
   * @return Activated item or nullptr when activation is not allowed.
   */
  const MenuItem* ActivateSelected() const;

  /**
   * @brief Builds a readable dump of the menu state.
   *
   * @return Human-readable state dump.
   */
  std::string Dump() const;

 private:
  /**
   * @brief Finds first enabled index.
   *
   * @return Result produced by the operation, when applicable.
   */
  int FindFirstEnabledIndex() const;
  /**
   * @brief Finds the next enabled menu item index in the requested navigation direction.
   *
   * @param start_index Input value required by the operation.
   * @param direction Input value required by the operation.
   * @return Result produced by the operation, when applicable.
   */
  int FindNextEnabledIndex(int start_index, int direction) const;

  std::vector<MenuItem> items_;
  int selected_index_ = -1;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_UI_MAIN_MENU_H_
