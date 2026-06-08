/**
 * @file src/developer/developer_config.cpp
 * @brief Developer-only configuration and diagnostic settings. Contains implementation for
 * developer_config.cpp.
 */

#include "developer/developer_config.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace sar {
namespace {

/**
 * @brief Stores parse string result data shared between runtime systems.
 */
struct ParseStringResult {
  bool ok = false;
  std::string value;
  std::string error;
};

/**
 * @brief Stores parse bool result data shared between runtime systems.
 */
struct ParseBoolResult {
  bool ok = false;
  bool found = false;
  bool value = false;
  std::string error;
};

/**
 * @brief Reads text file.
 */
std::string ReadTextFile(const std::filesystem::path& path,
                         std::string* error) {
  std::ifstream input(path);
  if (!input.is_open()) {
    *error = "failed to open developer config file: " + path.string();
    return {};
  }

  std::ostringstream stream;
  stream << input.rdbuf();
  return stream.str();
}

/**
 * @brief Executes the skip whitespace operation.
 */
void SkipWhitespace(std::string_view text, std::size_t* position) {
  while (*position < text.size() &&
         std::isspace(static_cast<unsigned char>(text[*position])) != 0) {
    ++(*position);
  }
}

/**
 * @brief Parses JSON string from external data.
 */
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

/**
 * @brief Finds field value start.
 */
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

/**
 * @brief Executes the extract optional JSON string field operation.
 */
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

/**
 * @brief Executes the extract optional JSON bool field operation.
 */
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

/**
 * @brief Executes the extract JSON array operation.
 */
std::optional<std::string_view> ExtractJsonArray(
    std::string_view text, std::string_view field_name, std::string* error) {
  const std::optional<std::size_t> value_start =
      FindFieldValueStart(text, field_name, error);
  if (!value_start.has_value()) {
    return std::nullopt;
  }
  if (*value_start >= text.size() || text[*value_start] != '[') {
    *error = "expected array value for field: " + std::string(field_name);
    return std::nullopt;
  }

  std::size_t position = *value_start;
  int depth = 0;
  bool in_string = false;
  bool escaped = false;
  for (; position < text.size(); ++position) {
    const char current = text[position];
    if (in_string) {
      if (escaped) {
        escaped = false;
      } else if (current == '\\') {
        escaped = true;
      } else if (current == '"') {
        in_string = false;
      }
      continue;
    }

    if (current == '"') {
      in_string = true;
      continue;
    }
    if (current == '[') {
      ++depth;
    } else if (current == ']') {
      --depth;
      if (depth == 0) {
        return text.substr(*value_start, position - *value_start + 1);
      }
    }
  }

  *error = "unterminated array field: " + std::string(field_name);
  return std::nullopt;
}

/**
 * @brief Executes the extract JSON objects from array operation.
 */
std::vector<std::string_view> ExtractJsonObjectsFromArray(
    std::string_view array_text) {
  std::vector<std::string_view> objects;
  int depth = 0;
  bool in_string = false;
  bool escaped = false;
  std::size_t object_start = std::string_view::npos;

  for (std::size_t position = 0; position < array_text.size(); ++position) {
    const char current = array_text[position];
    if (in_string) {
      if (escaped) {
        escaped = false;
      } else if (current == '\\') {
        escaped = true;
      } else if (current == '"') {
        in_string = false;
      }
      continue;
    }

    if (current == '"') {
      in_string = true;
      continue;
    }
    if (current == '{') {
      if (depth == 0) {
        object_start = position;
      }
      ++depth;
    } else if (current == '}') {
      --depth;
      if (depth == 0 && object_start != std::string_view::npos) {
        objects.push_back(array_text.substr(object_start,
                                            position - object_start + 1));
        object_start = std::string_view::npos;
      }
    }
  }

  return objects;
}

/**
 * @brief Applies optional bool field.
 */
bool ApplyOptionalBoolField(std::string_view text, std::string_view field_name,
                            bool* value, std::string* error) {
  const ParseBoolResult parsed = ExtractOptionalJsonBoolField(text, field_name);
  if (!parsed.ok) {
    *error = parsed.error;
    return false;
  }
  if (parsed.found) {
    *value = parsed.value;
  }
  return true;
}

/**
 * @brief Applies optional string field.
 */
bool ApplyOptionalStringField(std::string_view text, std::string_view field_name,
                              std::string* value, std::string* error) {
  const ParseStringResult parsed = ExtractOptionalJsonStringField(text,
                                                                  field_name);
  if (!parsed.ok) {
    *error = parsed.error;
    return false;
  }
  if (!parsed.value.empty()) {
    *value = parsed.value;
  }
  return true;
}

}  // namespace

/**
 * @brief Builds a readable diagnostic dump for dump.
 */
std::string DeveloperConfig::Dump() const {
  return "DeveloperConfig { log: { enabled: " +
         std::string(log.enabled ? "true" : "false") +
         ", color_enabled: " +
         std::string(log.color_enabled ? "true" : "false") +
         ", show_execution_context: " +
         std::string(log.show_execution_context ? "true" : "false") +
         ", visual_pipeline_diagnostics: " +
         std::string(log.visual_pipeline_diagnostics ? "true" : "false") +
         ", visual_pipeline_summary: " +
         std::string(log.visual_pipeline_summary ? "true" : "false") +
         ", visual_pipeline_step_details: " +
         std::string(log.visual_pipeline_step_details ? "true" : "false") +
         ", highlight_rules: " + std::to_string(log.highlight_rules.size()) +
         " } }";
}

/**
 * @brief Loads developer config.
 */
DeveloperConfigResult LoadDeveloperConfig(
    const std::filesystem::path& config_path) {
  std::error_code file_error;
  if (!std::filesystem::exists(config_path, file_error)) {
    if (file_error) {
      return {false, false, {},
              "failed to inspect developer config file: " +
                  file_error.message()};
    }
    return {true, false, {}, {}};
  }

  std::string error;
  const std::string content = ReadTextFile(config_path, &error);
  if (!error.empty()) {
    return {false, true, {}, error};
  }

  DeveloperConfig config;
  if (!ApplyOptionalBoolField(content, "enabled", &config.log.enabled,
                              &error) ||
      !ApplyOptionalBoolField(content, "color_enabled",
                              &config.log.color_enabled, &error) ||
      !ApplyOptionalBoolField(content, "show_execution_context",
                              &config.log.show_execution_context, &error) ||
      !ApplyOptionalBoolField(content, "visual_pipeline_diagnostics",
                              &config.log.visual_pipeline_diagnostics,
                              &error) ||
      !ApplyOptionalBoolField(content, "visual_pipeline_summary",
                              &config.log.visual_pipeline_summary, &error) ||
      !ApplyOptionalBoolField(content, "visual_pipeline_step_details",
                              &config.log.visual_pipeline_step_details,
                              &error)) {
    return {false, true, {}, error};
  }

  const std::optional<std::string_view> rules_text =
      ExtractJsonArray(content, "highlight_rules", &error);
  if (!error.empty()) {
    return {false, true, {}, error};
  }

  if (rules_text.has_value()) {
    for (std::string_view object_text :
         ExtractJsonObjectsFromArray(*rules_text)) {
      DeveloperHighlightRuleConfig rule;
      if (!ApplyOptionalStringField(object_text, "name", &rule.name,
                                    &error) ||
          !ApplyOptionalStringField(object_text, "regex",
                                    &rule.regex_pattern, &error) ||
          !ApplyOptionalStringField(object_text, "color", &rule.color,
                                    &error) ||
          !ApplyOptionalStringField(object_text, "scope", &rule.scope,
                                    &error) ||
          !ApplyOptionalBoolField(object_text, "case_sensitive",
                                  &rule.case_sensitive, &error)) {
        return {false, true, {}, error};
      }

      if (rule.regex_pattern.empty() || rule.color.empty()) {
        return {false, true, {},
                "developer highlight rule requires regex and color"};
      }
      config.log.highlight_rules.push_back(std::move(rule));
    }
  }

  return {true, true, config, {}};
}

}  // namespace sar
