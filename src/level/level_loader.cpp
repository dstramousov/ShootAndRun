#include "level/level_loader.h"

#include <charconv>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace sar {
namespace {

struct ReadFileResult {
  bool ok = false;
  std::string content;
  std::string error;
};

struct IntFieldResult {
  bool ok = false;
  int value = 0;
  std::string error;
};

struct GridShapeResult {
  bool ok = false;
  int rows = 0;
  int columns = 0;
  std::string field_name;
  std::string error;
};

ReadFileResult ReadTextFile(const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input.is_open()) {
    return {false, {}, "failed to open file: " + path.string()};
  }

  std::ostringstream stream;
  stream << input.rdbuf();
  return {true, stream.str(), {}};
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

IntFieldResult ExtractRequiredIntField(std::string_view text,
                                       std::string_view field_name) {
  std::string error;
  const std::optional<std::size_t> value_start =
      FindFieldValueStart(text, field_name, &error);
  if (!value_start.has_value()) {
    if (!error.empty()) {
      return {false, 0, error};
    }
    return {false, 0, "missing required field: " + std::string(field_name)};
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
    return {false, 0,
            "expected integer value for field: " + std::string(field_name)};
  }

  int value = 0;
  const auto result = std::from_chars(text.data() + *value_start,
                                      text.data() + end, value);
  if (result.ec != std::errc()) {
    return {false, 0,
            "invalid integer value for field: " + std::string(field_name)};
  }

  return {true, value, {}};
}

bool SkipJsonString(std::string_view text, std::size_t* position,
                    std::string* error) {
  if (*position >= text.size() || text[*position] != '"') {
    *error = "expected JSON string";
    return false;
  }

  ++(*position);
  while (*position < text.size()) {
    const char current = text[(*position)++];
    if (current == '"') {
      return true;
    }

    if (current != '\\') {
      continue;
    }

    if (*position >= text.size()) {
      *error = "unterminated JSON escape sequence";
      return false;
    }

    ++(*position);
  }

  *error = "unterminated JSON string";
  return false;
}

bool SkipJsonValue(std::string_view text, std::size_t* position,
                   std::string* error);

bool SkipJsonArray(std::string_view text, std::size_t* position,
                   std::string* error) {
  if (*position >= text.size() || text[*position] != '[') {
    *error = "expected JSON array";
    return false;
  }

  ++(*position);
  SkipWhitespace(text, position);
  if (*position < text.size() && text[*position] == ']') {
    ++(*position);
    return true;
  }

  while (*position < text.size()) {
    if (!SkipJsonValue(text, position, error)) {
      return false;
    }

    SkipWhitespace(text, position);
    if (*position >= text.size()) {
      *error = "unterminated JSON array";
      return false;
    }

    if (text[*position] == ',') {
      ++(*position);
      SkipWhitespace(text, position);
      continue;
    }

    if (text[*position] == ']') {
      ++(*position);
      return true;
    }

    *error = "expected comma or array close bracket";
    return false;
  }

  *error = "unterminated JSON array";
  return false;
}

bool SkipJsonObject(std::string_view text, std::size_t* position,
                    std::string* error) {
  if (*position >= text.size() || text[*position] != '{') {
    *error = "expected JSON object";
    return false;
  }

  ++(*position);
  SkipWhitespace(text, position);
  if (*position < text.size() && text[*position] == '}') {
    ++(*position);
    return true;
  }

  while (*position < text.size()) {
    if (!SkipJsonString(text, position, error)) {
      return false;
    }

    SkipWhitespace(text, position);
    if (*position >= text.size() || text[*position] != ':') {
      *error = "expected colon after object key";
      return false;
    }

    ++(*position);
    SkipWhitespace(text, position);
    if (!SkipJsonValue(text, position, error)) {
      return false;
    }

    SkipWhitespace(text, position);
    if (*position >= text.size()) {
      *error = "unterminated JSON object";
      return false;
    }

    if (text[*position] == ',') {
      ++(*position);
      SkipWhitespace(text, position);
      continue;
    }

    if (text[*position] == '}') {
      ++(*position);
      return true;
    }

    *error = "expected comma or object close bracket";
    return false;
  }

  *error = "unterminated JSON object";
  return false;
}

bool SkipJsonPrimitive(std::string_view text, std::size_t* position,
                       std::string* error) {
  const std::size_t start = *position;
  while (*position < text.size()) {
    const char current = text[*position];
    if (current == ',' || current == ']' || current == '}' ||
        std::isspace(static_cast<unsigned char>(current)) != 0) {
      break;
    }
    ++(*position);
  }

  if (start == *position) {
    *error = "expected JSON value";
    return false;
  }

  return true;
}

bool SkipJsonValue(std::string_view text, std::size_t* position,
                   std::string* error) {
  SkipWhitespace(text, position);
  if (*position >= text.size()) {
    *error = "expected JSON value";
    return false;
  }

  switch (text[*position]) {
    case '"':
      return SkipJsonString(text, position, error);
    case '[':
      return SkipJsonArray(text, position, error);
    case '{':
      return SkipJsonObject(text, position, error);
    default:
      return SkipJsonPrimitive(text, position, error);
  }
}

GridShapeResult ParseGridShapeAt(std::string_view text, std::size_t position,
                                 std::string_view field_name) {
  if (position >= text.size() || text[position] != '[') {
    return {false, 0, 0, std::string(field_name),
            "expected grid array for field: " + std::string(field_name)};
  }

  int rows = 0;
  int expected_columns = -1;
  ++position;
  SkipWhitespace(text, &position);

  if (position < text.size() && text[position] == ']') {
    return {false, 0, 0, std::string(field_name),
            "grid must not be empty: " + std::string(field_name)};
  }

  while (position < text.size()) {
    if (text[position] != '[') {
      return {false, 0, 0, std::string(field_name),
              "expected grid row array for field: " +
                  std::string(field_name)};
    }

    ++position;
    SkipWhitespace(text, &position);
    int columns = 0;

    if (position < text.size() && text[position] != ']') {
      while (position < text.size()) {
        std::string error;
        if (!SkipJsonValue(text, &position, &error)) {
          return {false, 0, 0, std::string(field_name), error};
        }

        ++columns;
        SkipWhitespace(text, &position);

        if (position >= text.size()) {
          return {false, 0, 0, std::string(field_name),
                  "unterminated grid row: " + std::string(field_name)};
        }

        if (text[position] == ',') {
          ++position;
          SkipWhitespace(text, &position);
          continue;
        }

        if (text[position] == ']') {
          break;
        }

        return {false, 0, 0, std::string(field_name),
                "expected comma or row close bracket: " +
                    std::string(field_name)};
      }
    }

    if (position >= text.size() || text[position] != ']') {
      return {false, 0, 0, std::string(field_name),
              "unterminated grid row: " + std::string(field_name)};
    }
    ++position;

    if (columns <= 0) {
      return {false, 0, 0, std::string(field_name),
              "grid row must not be empty: " + std::string(field_name)};
    }

    if (expected_columns < 0) {
      expected_columns = columns;
    } else if (columns != expected_columns) {
      return {false, 0, 0, std::string(field_name),
              "grid rows have different widths: " +
                  std::string(field_name)};
    }

    ++rows;
    SkipWhitespace(text, &position);

    if (position >= text.size()) {
      return {false, 0, 0, std::string(field_name),
              "unterminated grid array: " + std::string(field_name)};
    }

    if (text[position] == ',') {
      ++position;
      SkipWhitespace(text, &position);
      continue;
    }

    if (text[position] == ']') {
      ++position;
      return {true, rows, expected_columns, std::string(field_name), {}};
    }

    return {false, 0, 0, std::string(field_name),
            "expected comma or grid close bracket: " +
                std::string(field_name)};
  }

  return {false, 0, 0, std::string(field_name),
          "unterminated grid array: " + std::string(field_name)};
}

GridShapeResult ExtractGridShape(
    std::string_view text, const std::vector<std::string_view>& field_names) {
  std::string field_error;
  for (const std::string_view field_name : field_names) {
    const std::optional<std::size_t> value_start =
        FindFieldValueStart(text, field_name, &field_error);
    if (!value_start.has_value()) {
      if (!field_error.empty()) {
        return {false, 0, 0, std::string(field_name), field_error};
      }
      continue;
    }

    return ParseGridShapeAt(text, *value_start, field_name);
  }

  return {false, 0, 0, {}, "missing required grid field"};
}

bool ValidateGridShape(const GridShapeResult& grid, int expected_width,
                       int expected_height, std::string* error) {
  if (!grid.ok) {
    *error = grid.error;
    return false;
  }

  if (grid.columns != expected_width || grid.rows != expected_height) {
    *error = "grid size mismatch for " + grid.field_name + ": expected=" +
             std::to_string(expected_width) + "x" +
             std::to_string(expected_height) + " actual=" +
             std::to_string(grid.columns) + "x" + std::to_string(grid.rows);
    return false;
  }

  return true;
}

}  // namespace

std::string LevelPackageSummary::Dump() const {
  return "LevelPackageSummary { path: \"" + package_path.string() +
         "\", width: " + std::to_string(size.width) +
         ", height: " + std::to_string(size.height) +
         ", tile_size: " + std::to_string(size.tile_size) +
         ", runtime_grids: " +
         std::to_string(validated_runtime_grid_count) + " }";
}

LevelLoadResult LevelLoader::LoadBasicPackage(
    const std::filesystem::path& package_path) const {
  const std::filesystem::path terrain_path = package_path / "terrain.json";
  const std::filesystem::path runtime_grids_path =
      package_path / "runtime_grids.json";

  std::error_code error_code;
  if (!std::filesystem::is_directory(package_path, error_code)) {
    return {false, {}, "map package path is not a directory: " +
                           package_path.string()};
  }

  if (error_code) {
    return {false, {}, "failed to inspect map package path: " +
                           package_path.string() +
                           " reason=" + error_code.message()};
  }

  const ReadFileResult terrain_file = ReadTextFile(terrain_path);
  if (!terrain_file.ok) {
    return {false, {}, terrain_file.error};
  }

  const ReadFileResult runtime_grids_file = ReadTextFile(runtime_grids_path);
  if (!runtime_grids_file.ok) {
    return {false, {}, runtime_grids_file.error};
  }

  const IntFieldResult width = ExtractRequiredIntField(terrain_file.content,
                                                       "width");
  if (!width.ok || width.value <= 0) {
    return {false, {}, width.ok ? "width must be positive" : width.error};
  }

  const IntFieldResult height = ExtractRequiredIntField(terrain_file.content,
                                                        "height");
  if (!height.ok || height.value <= 0) {
    return {false, {}, height.ok ? "height must be positive" : height.error};
  }

  const IntFieldResult tile_size = ExtractRequiredIntField(terrain_file.content,
                                                           "tile_size");
  if (!tile_size.ok || tile_size.value <= 0) {
    return {false, {},
            tile_size.ok ? "tile_size must be positive" : tile_size.error};
  }

  const GridShapeResult terrain_grid = ExtractGridShape(
      terrain_file.content, {"terrain_grid", "terrain", "grid"});
  std::string error;
  if (!ValidateGridShape(terrain_grid, width.value, height.value, &error)) {
    return {false, {}, error};
  }

  const IntFieldResult runtime_width = ExtractRequiredIntField(
      runtime_grids_file.content, "width");
  if (!runtime_width.ok || runtime_width.value != width.value) {
    return {false, {}, runtime_width.ok ? "runtime width mismatch"
                                        : runtime_width.error};
  }

  const IntFieldResult runtime_height = ExtractRequiredIntField(
      runtime_grids_file.content, "height");
  if (!runtime_height.ok || runtime_height.value != height.value) {
    return {false, {}, runtime_height.ok ? "runtime height mismatch"
                                         : runtime_height.error};
  }

  const std::vector<std::string_view> runtime_grids = {
      "movement_grid", "collision_grid", "projectile_block_grid",
      "vision_block_grid", "cover_grid", "concealment_grid", "height_grid"};

  int validated_runtime_grid_count = 0;
  for (const std::string_view grid_name : runtime_grids) {
    const GridShapeResult grid = ExtractGridShape(runtime_grids_file.content,
                                                  {grid_name});
    if (!ValidateGridShape(grid, width.value, height.value, &error)) {
      return {false, {}, error};
    }
    ++validated_runtime_grid_count;
  }

  LevelPackageSummary summary;
  summary.package_path = package_path;
  summary.size = LevelSize{width.value, height.value, tile_size.value};
  summary.validated_runtime_grid_count = validated_runtime_grid_count;
  return {true, summary, {}};
}

}  // namespace sar
