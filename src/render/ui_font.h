#ifndef SHOOT_AND_RUN_CPP_SRC_RENDER_UI_FONT_H_
#define SHOOT_AND_RUN_CPP_SRC_RENDER_UI_FONT_H_

/**
 * @file src/render/ui_font.h
 * @brief 2D/debug rendering helpers retained by the application shell. Contains public
 * declarations for ui_font.h.
 */

#include <raylib.h>

#include <filesystem>
#include <string>
#include <string_view>

namespace sar {

/**
 * @brief Owns the UI font behavior and its runtime state.
 */
class UiFont {
 public:
  /**
   * @brief Creates an empty font resource.
   */
  UiFont() = default;

  /**
   * @brief Executes the UI font operation.
   *
   * @param UiFont Input value required by the operation.
   */
  UiFont(const UiFont&) = delete;
  /**
   * @brief Executes the operator operation.
   *
   * @param UiFont Input value required by the operation.
   * @return Result produced by the operation, when applicable.
   */
  UiFont& operator=(const UiFont&) = delete;

  /**
   * @brief Moves a font resource from another instance.
   *
   * @param other Source font resource.
   */
  UiFont(UiFont&& other) noexcept;

  /**
   * @brief Moves a font resource from another instance.
   *
   * @param other Source font resource.
   * @return Reference to this instance.
   */
  UiFont& operator=(UiFont&& other) noexcept;

  /**
   * @brief Releases the loaded font resource.
   */
  ~UiFont();

  /**
   * @brief Loads a font from a file.
   *
   * @param font_path Path to the font file.
   * @param base_font_size Base font size in pixels.
   * @param error Output error text when loading fails.
   * @return true when the font was loaded successfully.
   */
  bool Load(const std::filesystem::path& font_path, int base_font_size,
            std::string* error);

  /**
   * @brief Releases the loaded font and returns to fallback rendering.
   */
  void Reset();

  /**
   * @brief Checks whether a custom font is loaded.
   *
   * @return true if custom font rendering is available.
   */
  bool is_loaded() const;

  /**
   * @brief Returns the configured base font size.
   *
   * @return Base font size in pixels.
   */
  int base_size() const;

  /**
   * @brief Measures text width using the active font.
   *
   * @param text Text to measure.
   * @param font_size Font size in pixels.
   * @return Text width in pixels.
   */
  int MeasureTextWidth(std::string_view text, int font_size) const;

  /**
   * @brief Draws a single text line using the active font.
   *
   * @param text Text to draw.
   * @param x X position in pixels.
   * @param y Y position in pixels.
   * @param font_size Font size in pixels.
   * @param color Text color.
   */
  void DrawTextLine(std::string_view text, int x, int y, int font_size,
                    Color color) const;

 private:
  Font font_{};
  bool loaded_ = false;
  int base_size_ = 24;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_RENDER_UI_FONT_H_
