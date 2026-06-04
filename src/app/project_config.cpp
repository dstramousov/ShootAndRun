#include "app/project_config.h"

#include <cctype>
#include <charconv>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace sar {
namespace {

struct ParseStringResult {
  bool ok = false;
  std::string value;
  std::string error;
};

struct ParseIntResult {
  bool ok = false;
  bool found = false;
  int value = 0;
  std::string error;
};

std::string ReadTextFile(const std::filesystem::path& path,
                         std::string* error) {
  std::ifstream input(path);
  if (!input.is_open()) {
    *error = "failed to open config file: " + path.string();
    return {};
  }

  std::ostringstream stream;
  stream << input.rdbuf();
  return stream.str();
}

void SkipWhitespace(std::string_view text, std::size_t* position) {
  while (*position < text.size() &&
         std::isspace(static_cast<unsigned char>(text[*position])) != 0) {
    ++(*position);
  }
}

std::optional<std::size_t> FindFieldValueStart(std::string_view text,
                                               std::string_view field_name,
                                               std::string* error) {
  const std::string key = '"' + std::string(field_name) + '"';
  const std::size_t key_position = text.find(key);
  if (key_position == std::string_view::npos) {
    return std::nullopt;
  }

  std::size_t position = key_position + key.size();
  SkipWhitespace(text, &position);

  if (position >= text.size() || text[position] != ':') {
    *error = "missing colon after field: " + std::string(field_name);
    return std::nullopt;
  }

  ++position;
  SkipWhitespace(text, &position);
  return position;
}

ParseStringResult ParseJsonString(std::string_view text,
                                  std::size_t position) {
  if (position >= text.size() || text[position] != '"') {
    return {false, {}, "expected string value"};
  }

  std::string value;
  ++position;

  while (position < text.size()) {
    const char current = text[position++];
    if (current == '"') {
      return {true, value, {}};
    }

    if (current != '\\') {
      value.push_back(current);
      continue;
    }

    if (position >= text.size()) {
      return {false, {}, "unterminated escape sequence"};
    }

    const char escaped = text[position++];
    switch (escaped) {
      case '"':
      case '\\':
      case '/':
        value.push_back(escaped);
        break;
      case 'b':
        value.push_back('\b');
        break;
      case 'f':
        value.push_back('\f');
        break;
      case 'n':
        value.push_back('\n');
        break;
      case 'r':
        value.push_back('\r');
        break;
      case 't':
        value.push_back('\t');
        break;
      default:
        return {false, {}, "unsupported escape sequence"};
    }
  }

  return {false, {}, "unterminated string value"};
}

ParseStringResult ExtractRequiredJsonStringField(
    std::string_view text, std::string_view field_name) {
  std::string error;
  const std::optional<std::size_t> value_start =
      FindFieldValueStart(text, field_name, &error);
  if (!value_start.has_value()) {
    if (!error.empty()) {
      return {false, {}, error};
    }
    return {false, {}, "missing required field: " + std::string(field_name)};
  }

  return ParseJsonString(text, *value_start);
}

ParseStringResult ExtractOptionalJsonStringField(
    std::string_view text, std::string_view field_name) {
  std::string error;
  const std::optional<std::size_t> value_start =
      FindFieldValueStart(text, field_name, &error);
  if (!value_start.has_value()) {
    if (!error.empty()) {
      return {false, {}, error};
    }
    return {true, {}, {}};
  }

  return ParseJsonString(text, *value_start);
}

ParseIntResult ExtractOptionalJsonIntField(std::string_view text,
                                           std::string_view field_name) {
  std::string error;
  const std::optional<std::size_t> value_start =
      FindFieldValueStart(text, field_name, &error);
  if (!value_start.has_value()) {
    if (!error.empty()) {
      return {false, false, 0, error};
    }
    return {true, false, 0, {}};
  }

  std::size_t end = *value_start;
  if (end < text.size() && text[end] == '-') {
    ++end;
  }

  const std::size_t digits_start = end;
  while (end < text.size() &&
         std::isdigit(static_cast<unsigned char>(text[end])) != 0) {
    ++end;
  }

  if (digits_start == end) {
    return {false, true, 0,
            "expected integer value for field: " + std::string(field_name)};
  }

  int value = 0;
  const auto result = std::from_chars(text.data() + *value_start,
                                      text.data() + end, value);
  if (result.ec != std::errc()) {
    return {false, true, 0,
            "invalid integer value for field: " + std::string(field_name)};
  }

  return {true, true, value, {}};
}

}  // namespace

std::string ProjectConfig::Dump() const {
  return "ProjectConfig { map_package_path: \"" +
         map_package_path.string() + "\", ui_font_path: \"" +
         ui_font_path.string() + "\", ui_font_size: " +
         std::to_string(ui_font_size) + " }";
}

ProjectConfigResult LoadProjectConfig(
    const std::filesystem::path& config_path) {
  std::string error;
  const std::string content = ReadTextFile(config_path, &error);
  if (!error.empty()) {
    return {false, {}, error};
  }

  ParseStringResult map_path =
      ExtractRequiredJsonStringField(content, "map_package_path");
  if (!map_path.ok) {
    return {false, {}, map_path.error};
  }

  if (map_path.value.empty()) {
    return {false, {}, "map_package_path must not be empty"};
  }

  ProjectConfig config;
  config.map_package_path = std::filesystem::path(map_path.value);

  ParseStringResult font_path =
      ExtractOptionalJsonStringField(content, "ui_font_path");
  if (!font_path.ok) {
    return {false, {}, font_path.error};
  }
  if (!font_path.value.empty()) {
    config.ui_font_path = std::filesystem::path(font_path.value);
  }

  ParseIntResult font_size = ExtractOptionalJsonIntField(content,
                                                         "ui_font_size");
  if (!font_size.ok) {
    return {false, {}, font_size.error};
  }
  if (font_size.found) {
    if (font_size.value <= 0) {
      return {false, {}, "ui_font_size must be positive"};
    }
    config.ui_font_size = font_size.value;
  }

  return {true, config, {}};
}

}  // namespace sar
