/**
 * @file src/ui/main_menu.cpp
 * @brief Main menu, confirmation dialog, and UI layout. Contains implementation for
 * main_menu.cpp.
 */

#include "ui/main_menu.h"

#include <sstream>

namespace sar {

/**
 * @brief Implements MainMenu::MainMenu.
 */
MainMenu::MainMenu()
    : items_({MenuItem{"new_game", "New Game", MenuAction::kNewGame, true},
              MenuItem{"load_game", "Load Game", MenuAction::kLoadGame, false},
              MenuItem{"save_game", "Save Game", MenuAction::kSaveGame, false},
              MenuItem{"settings", "Settings", MenuAction::kSettings, true},
              MenuItem{"exit", "Exit", MenuAction::kExit, true}}) {
  selected_index_ = FindFirstEnabledIndex();
}

const std::vector<MenuItem>& MainMenu::items() const { return items_; }

int MainMenu::selected_index() const { return selected_index_; }

/**
 * @brief Selects next.
 */
void MainMenu::SelectNext() {
  selected_index_ = FindNextEnabledIndex(selected_index_, 1);
}

/**
 * @brief Selects previous.
 */
void MainMenu::SelectPrevious() {
  selected_index_ = FindNextEnabledIndex(selected_index_, -1);
}

/**
 * @brief Selects index.
 */
bool MainMenu::SelectIndex(int index) {
  if (index < 0 || index >= static_cast<int>(items_.size()) ||
      !items_[static_cast<std::size_t>(index)].enabled) {
    return false;
  }

  if (selected_index_ == index) {
    return false;
  }

  selected_index_ = index;
  return true;
}

/**
 * @brief Selects ed item.
 */
const MenuItem* MainMenu::SelectedItem() const {
  if (selected_index_ < 0 || selected_index_ >= static_cast<int>(items_.size())) {
    return nullptr;
  }

  return &items_[static_cast<std::size_t>(selected_index_)];
}

/**
 * @brief Implements MainMenu::ActivateSelected.
 */
const MenuItem* MainMenu::ActivateSelected() const {
  const MenuItem* item = SelectedItem();
  if (item == nullptr || !item->enabled) {
    return nullptr;
  }

  return item;
}

/**
 * @brief Builds a readable diagnostic dump for dump.
 */
std::string MainMenu::Dump() const {
  std::ostringstream stream;
  stream << "MenuState { selected_index: " << selected_index_
         << ", item_count: " << items_.size() << " }";
  return stream.str();
}

/**
 * @brief Finds first enabled index.
 */
int MainMenu::FindFirstEnabledIndex() const {
  for (int i = 0; i < static_cast<int>(items_.size()); ++i) {
    if (items_[static_cast<std::size_t>(i)].enabled) {
      return i;
    }
  }

  return -1;
}

/**
 * @brief Finds the next enabled menu item index in the requested navigation direction.
 */
int MainMenu::FindNextEnabledIndex(int start_index, int direction) const {
  if (items_.empty()) {
    return -1;
  }

  int index = start_index;
  for (std::size_t step = 0; step < items_.size(); ++step) {
    index += direction;
    if (index < 0) {
      index = static_cast<int>(items_.size()) - 1;
    } else if (index >= static_cast<int>(items_.size())) {
      index = 0;
    }

    if (items_[static_cast<std::size_t>(index)].enabled) {
      return index;
    }
  }

  return start_index;
}

}  // namespace sar
