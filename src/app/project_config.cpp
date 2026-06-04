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

struct ParseFloatResult {
  bool ok = false;
  bool found = false;
  float value = 0.0F;
  std::string error;
};

struct ParseBoolResult {
  bool ok = false;
  bool found = false;
  bool value = false;
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

ParseFloatResult ExtractOptionalJsonFloatField(std::string_view text,
                                               std::string_view field_name) {
  std::string error;
  const std::optional<std::size_t> value_start =
      FindFieldValueStart(text, field_name, &error);
  if (!value_start.has_value()) {
    if (!error.empty()) {
      return {false, false, 0.0F, error};
    }
    return {true, false, 0.0F, {}};
  }

  std::size_t end = *value_start;
  while (end < text.size()) {
    const char current = text[end];
    const bool part_of_number =
        std::isdigit(static_cast<unsigned char>(current)) != 0 ||
        current == '-' || current == '+' || current == '.' ||
        current == 'e' || current == 'E';
    if (!part_of_number) {
      break;
    }
    ++end;
  }

  if (end == *value_start) {
    return {false, true, 0.0F,
            "expected number value for field: " + std::string(field_name)};
  }

  float value = 0.0F;
  const auto result = std::from_chars(text.data() + *value_start,
                                      text.data() + end, value);
  if (result.ec != std::errc()) {
    return {false, true, 0.0F,
            "invalid number value for field: " + std::string(field_name)};
  }

  return {true, true, value, {}};
}

ParseBoolResult ExtractOptionalJsonBoolField(std::string_view text,
                                             std::string_view field_name) {
  std::string error;
  const std::optional<std::size_t> value_start =
      FindFieldValueStart(text, field_name, &error);
  if (!value_start.has_value()) {
    if (!error.empty()) {
      return {false, false, false, error};
    }
    return {true, false, false, {}};
  }

  if (text.substr(*value_start, 4) == "true") {
    return {true, true, true, {}};
  }
  if (text.substr(*value_start, 5) == "false") {
    return {true, true, false, {}};
  }

  return {false, true, false,
          "expected boolean value for field: " + std::string(field_name)};
}

std::optional<RaylibLogLevel> ParseRaylibLogLevel(std::string_view value) {
  if (value == "trace") {
    return RaylibLogLevel::kTrace;
  }
  if (value == "debug") {
    return RaylibLogLevel::kDebug;
  }
  if (value == "info") {
    return RaylibLogLevel::kInfo;
  }
  if (value == "warning" || value == "warn") {
    return RaylibLogLevel::kWarning;
  }
  if (value == "error") {
    return RaylibLogLevel::kError;
  }
  if (value == "fatal") {
    return RaylibLogLevel::kFatal;
  }
  if (value == "none" || value == "off") {
    return RaylibLogLevel::kNone;
  }

  return std::nullopt;
}

}  // namespace

const char* RaylibLogLevelName(RaylibLogLevel level) {
  switch (level) {
    case RaylibLogLevel::kTrace:
      return "trace";
    case RaylibLogLevel::kDebug:
      return "debug";
    case RaylibLogLevel::kInfo:
      return "info";
    case RaylibLogLevel::kWarning:
      return "warning";
    case RaylibLogLevel::kError:
      return "error";
    case RaylibLogLevel::kFatal:
      return "fatal";
    case RaylibLogLevel::kNone:
      return "none";
  }

  return "warning";
}

std::string ProjectConfig::Dump() const {
  return "ProjectConfig { map_package_path: \"" +
         map_package_path.string() + "\", ui_font_path: \"" +
         ui_font_path.string() + "\", ui_font_size: " +
         std::to_string(ui_font_size) + ", raylib_log_level: " +
         RaylibLogLevelName(raylib_log_level) + ", window: " +
         std::to_string(window_config.preferred_width) + "x" +
         std::to_string(window_config.preferred_height) +
         ", fallback: " + std::to_string(window_config.fallback_width) +
         "x" + std::to_string(window_config.fallback_height) +
         ", max_monitor_fraction: " +
         std::to_string(window_config.max_monitor_fraction) +
         ", service_info: { enabled: " +
         std::string(service_info.enabled ? "true" : "false") +
         ", show_memory: " +
         std::string(service_info.show_memory ? "true" : "false") +
         ", update_interval_ms: " +
         std::to_string(service_info.update_interval_ms) + " } }";
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

  ParseStringResult raylib_log_level =
      ExtractOptionalJsonStringField(content, "raylib_log_level");
  if (!raylib_log_level.ok) {
    return {false, {}, raylib_log_level.error};
  }
  if (!raylib_log_level.value.empty()) {
    const std::optional<RaylibLogLevel> parsed =
        ParseRaylibLogLevel(raylib_log_level.value);
    if (!parsed.has_value()) {
      return {false, {},
              "unsupported raylib_log_level: " + raylib_log_level.value};
    }
    config.raylib_log_level = *parsed;
  }

  ParseIntResult preferred_width =
      ExtractOptionalJsonIntField(content, "preferred_width");
  if (!preferred_width.ok) {
    return {false, {}, preferred_width.error};
  }
  if (preferred_width.found) {
    if (preferred_width.value <= 0) {
      return {false, {}, "preferred_width must be positive"};
    }
    config.window_config.preferred_width = preferred_width.value;
  }

  ParseIntResult preferred_height =
      ExtractOptionalJsonIntField(content, "preferred_height");
  if (!preferred_height.ok) {
    return {false, {}, preferred_height.error};
  }
  if (preferred_height.found) {
    if (preferred_height.value <= 0) {
      return {false, {}, "preferred_height must be positive"};
    }
    config.window_config.preferred_height = preferred_height.value;
  }

  ParseIntResult fallback_width =
      ExtractOptionalJsonIntField(content, "fallback_width");
  if (!fallback_width.ok) {
    return {false, {}, fallback_width.error};
  }
  if (fallback_width.found) {
    if (fallback_width.value <= 0) {
      return {false, {}, "fallback_width must be positive"};
    }
    config.window_config.fallback_width = fallback_width.value;
  }

  ParseIntResult fallback_height =
      ExtractOptionalJsonIntField(content, "fallback_height");
  if (!fallback_height.ok) {
    return {false, {}, fallback_height.error};
  }
  if (fallback_height.found) {
    if (fallback_height.value <= 0) {
      return {false, {}, "fallback_height must be positive"};
    }
    config.window_config.fallback_height = fallback_height.value;
  }

  ParseFloatResult max_monitor_fraction =
      ExtractOptionalJsonFloatField(content, "max_monitor_fraction");
  if (!max_monitor_fraction.ok) {
    return {false, {}, max_monitor_fraction.error};
  }
  if (max_monitor_fraction.found) {
    if (max_monitor_fraction.value <= 0.0F ||
        max_monitor_fraction.value > 1.0F) {
      return {false, {}, "max_monitor_fraction must be in range (0, 1]"};
    }
    config.window_config.max_monitor_fraction = max_monitor_fraction.value;
  }

  ParseBoolResult resizable =
      ExtractOptionalJsonBoolField(content, "resizable");
  if (!resizable.ok) {
    return {false, {}, resizable.error};
  }
  if (resizable.found) {
    config.window_config.resizable = resizable.value;
  }

  ParseBoolResult service_info_enabled =
      ExtractOptionalJsonBoolField(content, "enabled");
  if (!service_info_enabled.ok) {
    return {false, {}, service_info_enabled.error};
  }
  if (service_info_enabled.found) {
    config.service_info.enabled = service_info_enabled.value;
  }

  ParseBoolResult show_memory =
      ExtractOptionalJsonBoolField(content, "show_memory");
  if (!show_memory.ok) {
    return {false, {}, show_memory.error};
  }
  if (show_memory.found) {
    config.service_info.show_memory = show_memory.value;
  }

  ParseIntResult update_interval_ms =
      ExtractOptionalJsonIntField(content, "update_interval_ms");
  if (!update_interval_ms.ok) {
    return {false, {}, update_interval_ms.error};
  }
  if (update_interval_ms.found) {
    if (update_interval_ms.value <= 0) {
      return {false, {}, "update_interval_ms must be positive"};
    }
    config.service_info.update_interval_ms = update_interval_ms.value;
  }

  return {true, config, {}};
}

}  // namespace sar
