/**
 * @file src/render/ui_font.cpp
 * @brief 2D/debug rendering helpers retained by the application shell. Contains implementation
 * for ui_font.cpp.
 */

#include "render/ui_font.h"

#include <raylib.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

namespace sar {
namespace {

constexpr float kFontSpacing = 1.0F;

/**
 * @brief Executes the copy text operation.
 */
std::string CopyText(std::string_view text) {
  return std::string(text.data(), text.size());
}

}  // namespace

/**
 * @brief Implements UiFont::UiFont.
 */
UiFont::UiFont(UiFont&& other) noexcept
    : font_(other.font_), loaded_(other.loaded_), base_size_(other.base_size_) {
  other.font_ = Font{};
  other.loaded_ = false;
}

/**
 * @brief Executes the operator operation.
 */
UiFont& UiFont::operator=(UiFont&& other) noexcept {
  if (this == &other) {
    return *this;
  }

  Reset();
  font_ = other.font_;
  loaded_ = other.loaded_;
  base_size_ = other.base_size_;
  other.font_ = Font{};
  other.loaded_ = false;
  return *this;
}

UiFont::~UiFont() { Reset(); }

/**
 * @brief Loads runtime data.
 */
bool UiFont::Load(const std::filesystem::path& font_path, int base_font_size,
                  std::string* error) {
  Reset();

  if (base_font_size <= 0) {
    *error = "font size must be positive";
    return false;
  }

  std::error_code error_code;
  if (!std::filesystem::exists(font_path, error_code)) {
    *error = "font file does not exist: " + font_path.string();
    return false;
  }

  if (error_code) {
    *error = "failed to inspect font file: " + font_path.string() +
             " reason=" + error_code.message();
    return false;
  }

  font_ = LoadFontEx(font_path.string().c_str(), base_font_size, nullptr, 0);
  if (font_.texture.id == 0) {
    *error = "raylib failed to load font: " + font_path.string();
    font_ = Font{};
    return false;
  }

  loaded_ = true;
  base_size_ = base_font_size;
  return true;
}

/**
 * @brief Resets runtime state to defaults.
 */
void UiFont::Reset() {
  if (loaded_) {
    UnloadFont(font_);
    font_ = Font{};
    loaded_ = false;
  }
}

bool UiFont::is_loaded() const { return loaded_; }

int UiFont::base_size() const { return base_size_; }

/**
 * @brief Implements UiFont::MeasureTextWidth.
 */
int UiFont::MeasureTextWidth(std::string_view text, int font_size) const {
  const std::string copied_text = CopyText(text);
  if (!loaded_) {
    return MeasureText(copied_text.c_str(), font_size);
  }

  const Vector2 size = MeasureTextEx(font_, copied_text.c_str(),
                                     static_cast<float>(font_size),
                                     kFontSpacing);
  return static_cast<int>(size.x);
}

/**
 * @brief Draws text line.
 */
void UiFont::DrawTextLine(std::string_view text, int x, int y, int font_size,
                          Color color) const {
  const std::string copied_text = CopyText(text);
  if (!loaded_) {
    DrawText(copied_text.c_str(), x, y, font_size, color);
    return;
  }

  DrawTextEx(font_, copied_text.c_str(),
             Vector2{static_cast<float>(x), static_cast<float>(y)},
             static_cast<float>(font_size), kFontSpacing, color);
}

}  // namespace sar
