#ifndef SHOOT_AND_RUN_CPP_SRC_UI_CONFIRM_DIALOG_H_
#define SHOOT_AND_RUN_CPP_SRC_UI_CONFIRM_DIALOG_H_

/**
 * @file src/ui/confirm_dialog.h
 * @brief Main menu, confirmation dialog, and UI layout. Contains public declarations for
 * confirm_dialog.h.
 */

#include <string>

namespace sar {

/**
 * @brief Defines the supported dialog choice values.
 */
enum class DialogChoice {
  kNo,
  kYes,
};

/**
 * @brief Owns the confirm dialog behavior and its runtime state.
 */
class ConfirmDialog {
 public:
  /**
   * @brief Creates a confirmation dialog with a safe default choice.
   *
   * @param title Dialog title.
   * @param message Dialog message.
   */
  ConfirmDialog(std::string title, std::string message);

  /**
   * @brief Returns the dialog title.
   *
   * @return Dialog title.
   */
  const std::string& title() const;

  /**
   * @brief Returns the dialog message.
   *
   * @return Dialog message.
   */
  const std::string& message() const;

  /**
   * @brief Returns the selected dialog choice.
   *
   * @return Current selected choice.
   */
  DialogChoice selected_choice() const;

  /**
   * @brief Selects the positive choice.
   */
  void SelectYes();

  /**
   * @brief Selects the safe negative choice.
   */
  void SelectNo();

  /**
   * @brief Toggles selected choice.
   */
  void Toggle();

  /**
   * @brief Builds a readable dump of dialog state.
   *
   * @return Human-readable state dump.
   */
  std::string Dump() const;

 private:
  std::string title_;
  std::string message_;
  DialogChoice selected_choice_ = DialogChoice::kNo;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_UI_CONFIRM_DIALOG_H_
