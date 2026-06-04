#include "app/project_config.h"

#include <cctype>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace sar {
namespace {

struct ParseResult {
  bool ok = false;
  std::string value;
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

ParseResult ParseJsonString(std::string_view text, std::size_t position) {
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

ParseResult ExtractJsonStringField(std::string_view text,
                                   std::string_view field_name) {
  const std::string key = '"' + std::string(field_name) + '"';
  const std::size_t key_position = text.find(key);
  if (key_position == std::string_view::npos) {
    return {false, {}, "missing required field: " + std::string(field_name)};
  }

  std::size_t position = key_position + key.size();
  SkipWhitespace(text, &position);

  if (position >= text.size() || text[position] != ':') {
    return {false, {}, "missing colon after field: " + std::string(field_name)};
  }

  ++position;
  SkipWhitespace(text, &position);
  return ParseJsonString(text, position);
}

}  // namespace

std::string ProjectConfig::Dump() const {
  return "ProjectConfig { map_package_path: \"" +
         map_package_path.string() + "\" }";
}

ProjectConfigResult LoadProjectConfig(
    const std::filesystem::path& config_path) {
  std::string error;
  const std::string content = ReadTextFile(config_path, &error);
  if (!error.empty()) {
    return {false, {}, error};
  }

  ParseResult map_path = ExtractJsonStringField(content, "map_package_path");
  if (!map_path.ok) {
    return {false, {}, map_path.error};
  }

  if (map_path.value.empty()) {
    return {false, {}, "map_package_path must not be empty"};
  }

  ProjectConfig config;
  config.map_package_path = std::filesystem::path(map_path.value);
  return {true, config, {}};
}

}  // namespace sar
