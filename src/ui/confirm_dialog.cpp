#include "ui/confirm_dialog.h"

#include <sstream>
#include <utility>

namespace sar {

ConfirmDialog::ConfirmDialog(std::string title, std::string message)
    : title_(std::move(title)), message_(std::move(message)) {}

const std::string& ConfirmDialog::title() const { return title_; }

const std::string& ConfirmDialog::message() const { return message_; }

DialogChoice ConfirmDialog::selected_choice() const { return selected_choice_; }

void ConfirmDialog::SelectYes() { selected_choice_ = DialogChoice::kYes; }

void ConfirmDialog::SelectNo() { selected_choice_ = DialogChoice::kNo; }

void ConfirmDialog::Toggle() {
  selected_choice_ = selected_choice_ == DialogChoice::kYes ? DialogChoice::kNo
                                                            : DialogChoice::kYes;
}

std::string ConfirmDialog::Dump() const {
  std::ostringstream stream;
  stream << "ConfirmDialogState { title: \"" << title_ << "\", selected: "
         << (selected_choice_ == DialogChoice::kYes ? "yes" : "no") << " }";
  return stream.str();
}

}  // namespace sar
