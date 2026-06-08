/**
 * @file src/ui/confirm_dialog.cpp
 * @brief Main menu, confirmation dialog, and UI layout. Contains implementation for
 * confirm_dialog.cpp.
 */

#include "ui/confirm_dialog.h"

#include <sstream>
#include <utility>

namespace sar {

/**
 * @brief Implements ConfirmDialog::ConfirmDialog.
 */
ConfirmDialog::ConfirmDialog(std::string title, std::string message)
    : title_(std::move(title)), message_(std::move(message)) {}

const std::string& ConfirmDialog::title() const { return title_; }

const std::string& ConfirmDialog::message() const { return message_; }

DialogChoice ConfirmDialog::selected_choice() const { return selected_choice_; }

void ConfirmDialog::SelectYes() { selected_choice_ = DialogChoice::kYes; }

void ConfirmDialog::SelectNo() { selected_choice_ = DialogChoice::kNo; }

/**
 * @brief Converts to ggle.
 */
void ConfirmDialog::Toggle() {
  selected_choice_ = selected_choice_ == DialogChoice::kYes ? DialogChoice::kNo
                                                            : DialogChoice::kYes;
}

/**
 * @brief Builds a readable diagnostic dump for dump.
 */
std::string ConfirmDialog::Dump() const {
  std::ostringstream stream;
  stream << "ConfirmDialogState { title: \"" << title_ << "\", selected: "
         << (selected_choice_ == DialogChoice::kYes ? "yes" : "no") << " }";
  return stream.str();
}

}  // namespace sar
