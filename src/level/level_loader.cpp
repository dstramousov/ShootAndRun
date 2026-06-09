/**
 * @file src/level/level_loader.cpp
 * @brief Generated map package data contracts and loading logic. Contains implementation for
 * level_loader.cpp.
 */

#include "level/level_loader.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sar {
namespace {

constexpr int kDefaultTileSize = 16;

/**
 * @brief Stores read file result data shared between runtime systems.
 */
struct ReadFileResult {
  bool ok = false;
  std::string content;
  std::string error;
};

/**
 * @brief Stores int field result data shared between runtime systems.
 */
struct IntFieldResult {
  bool ok = false;
  bool found = false;
  int value = 0;
  std::string error;
};

/**
 * @brief Stores string field result data shared between runtime systems.
 */
struct StringFieldResult {
  bool ok = false;
  bool found = false;
  std::string value;
  std::string error;
};

/**
 * @brief Stores grid shape result data shared between runtime systems.
 */
struct GridShapeResult {
  bool ok = false;
  int rows = 0;
  int columns = 0;
  std::string field_name;
  std::string error;
};

/**
 * @brief Stores terrain grid result data shared between runtime systems.
 */
struct TerrainGridResult {
  TerrainGridResult() = default;

  TerrainGridResult(bool ok_value, int row_count, int column_count,
                    std::vector<TerrainType> terrain_cells,
                    std::string parsed_field_name, std::string error_text)
      : ok(ok_value),
        rows(row_count),
        columns(column_count),
        cells(std::move(terrain_cells)),
        field_name(std::move(parsed_field_name)),
        error(std::move(error_text)) {}

  bool ok = false;
  int rows = 0;
  int columns = 0;
  std::vector<TerrainType> cells;
  std::string field_name;
  std::string error;
  std::map<std::string, int> terrain_type_counts;
  std::map<std::string, int> unknown_terrain_type_counts;
  int catalog_type_count = 0;
  bool used_catalog = false;
};

/**
 * @brief Stores numeric grid result data shared between runtime systems.
 */
struct NumericGridResult {
  bool ok = false;
  int rows = 0;
  int columns = 0;
  std::vector<double> values;
  std::vector<std::uint8_t> present;
  std::string field_name;
  std::string error;
};

/**
 * @brief Stores tile catalog entry data shared between runtime systems.
 */
struct TileCatalogEntry {
  TerrainType terrain = TerrainType::kUnknown;
};

/**
 * @brief Stores tile catalog data shared between runtime systems.
 */
struct TileCatalog {
  std::map<std::string, TileCatalogEntry> entries;

  bool empty() const { return entries.empty(); }
};

/**
 * @brief Stores marker load result data shared between runtime systems.
 */
struct MarkerLoadResult {
  bool ok = false;
  std::vector<Marker> markers;
  std::string error;
};

/**
 * @brief Stores runtime object load result data shared between runtime systems.
 */
struct RuntimeObjectLoadResult {
  bool ok = false;
  std::vector<RuntimeObject> objects;
  std::string error;
};

/**
 * @brief Stores place load result data shared between runtime systems.
 */
struct PlaceLoadResult {
  bool ok = false;
  std::vector<Place> places;
  std::string error;
};

/**
 * @brief Stores route load result data shared between runtime systems.
 */
struct RouteLoadResult {
  bool ok = false;
  std::vector<Route> routes;
  std::string error;
};

/**
 * @brief Stores gameplay zone load result data shared between runtime systems.
 */
struct GameplayZoneLoadResult {
  bool ok = false;
  std::vector<GameplayZone> zones;
  std::string error;
};

/**
 * @brief Stores elevation transition load result data shared between runtime systems.
 */
struct ElevationTransitionLoadResult {
  bool ok = false;
  std::vector<ElevationTransition> transitions;
  std::string error;
};

/**
 * @brief Stores world graph load result data shared between runtime systems.
 */
struct WorldGraphLoadResult {
  bool ok = false;
  WorldGraph graph;
  std::string error;
};

/**
 * @brief Stores package layout data shared between runtime systems.
 */
bool ValidateElevationRange(int elevation, std::string_view source,
                            std::string* error);

struct PackageLayout {
  std::filesystem::path package_path;
  std::filesystem::path terrain_path;
  std::filesystem::path runtime_grids_path;
  std::filesystem::path markers_path;
  std::filesystem::path runtime_objects_path;
  std::filesystem::path places_path;
  std::filesystem::path routes_path;
  std::filesystem::path world_graph_path;
  std::filesystem::path gameplay_zones_path;
  std::filesystem::path elevation_transitions_path;
  std::filesystem::path tile_types_catalog_path;
  bool has_tile_types_catalog = false;
  bool uses_manifest = false;
  bool has_manifest_size = false;
  LevelSize manifest_size;
};


/**
 * @brief Executes the fallback movement multiplier operation.
 */
float FallbackMovementMultiplier(TerrainType terrain) {
  switch (terrain) {
    case TerrainType::kRoad:
      return 1.08F;
    case TerrainType::kSwamp:
    case TerrainType::kWater:
      return 0.50F;
    case TerrainType::kForest:
      return 0.55F;
    case TerrainType::kRuins:
      return 0.90F;
    case TerrainType::kOpenGround:
      return 1.0F;
    case TerrainType::kWall:
      return 0.0F;
    case TerrainType::kUnknown:
      return 0.80F;
  }
  return 1.0F;
}

/**
 * @brief Executes the normalize movement multiplier operation.
 */
float NormalizeMovementMultiplier(double grid_value, TerrainType terrain,
                                  bool walkable, bool collision) {
  if (collision || !walkable) {
    return 0.0F;
  }

  const float fallback = FallbackMovementMultiplier(terrain);
  if (grid_value > 0.0) {
    const float raw_value = std::clamp(static_cast<float>(grid_value),
                                       0.05F, 1.50F);
    if (fallback > 0.0F && fallback < 1.0F) {
      return std::min(raw_value, fallback);
    }
    return raw_value;
  }

  return fallback;
}

/**
 * @brief Reads text file.
 */
ReadFileResult ReadTextFile(const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input.is_open()) {
    return {false, {}, "failed to open file: " + path.string()};
  }

  std::ostringstream stream;
  stream << input.rdbuf();
  return {true, stream.str(), {}};
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
 * @brief Finds field value start.
 */
std::optional<std::size_t> FindFieldValueStart(std::string_view text,
                                               std::string_view field_name,
                                               std::string* /*error*/) {
  const std::string key = '"' + std::string(field_name) + '"';
  std::size_t search_position = 0;

  while (search_position < text.size()) {
    const std::size_t key_position = text.find(key, search_position);
    if (key_position == std::string_view::npos) {
      return std::nullopt;
    }

    std::size_t position = key_position + key.size();
    SkipWhitespace(text, &position);
    if (position < text.size() && text[position] == ':') {
      ++position;
      SkipWhitespace(text, &position);
      return position;
    }

    search_position = key_position + 1;
  }

  return std::nullopt;
}

/**
 * @brief Executes the extract optional int field operation.
 */
IntFieldResult ExtractOptionalIntField(std::string_view text,
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

/**
 * @brief Executes the extract required int field operation.
 */
IntFieldResult ExtractRequiredIntField(std::string_view text,
                                       std::string_view field_name) {
  IntFieldResult result = ExtractOptionalIntField(text, field_name);
  if (!result.ok) {
    return result;
  }

  if (!result.found) {
    return {false, false, 0,
            "missing required field: " + std::string(field_name)};
  }

  return result;
}

/**
 * @brief Returns skip JSON string.
 */
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

/**
 * @brief Parses JSON string at from external data.
 */
StringFieldResult ParseJsonStringAt(std::string_view text,
                                    std::size_t position,
                                    std::string_view field_name) {
  if (position >= text.size() || text[position] != '"') {
    return {false, true, {},
            "expected string value for field: " + std::string(field_name)};
  }

  ++position;
  std::string value;
  while (position < text.size()) {
    const char current = text[position++];
    if (current == '"') {
      return {true, true, value, {}};
    }

    if (current != '\\') {
      value.push_back(current);
      continue;
    }

    if (position >= text.size()) {
      return {false, true, {}, "unterminated JSON escape sequence"};
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
        return {false, true, {}, "unsupported JSON string escape"};
    }
  }

  return {false, true, {}, "unterminated JSON string"};
}

/**
 * @brief Executes the extract optional string field operation.
 */
StringFieldResult ExtractOptionalStringField(std::string_view text,
                                             std::string_view field_name) {
  std::string error;
  const std::optional<std::size_t> value_start =
      FindFieldValueStart(text, field_name, &error);
  if (!value_start.has_value()) {
    if (!error.empty()) {
      return {false, false, {}, error};
    }
    return {true, false, {}, {}};
  }

  return ParseJsonStringAt(text, *value_start, field_name);
}

/**
 * @brief Executes the extract required string field operation.
 */
StringFieldResult ExtractRequiredStringField(std::string_view text,
                                             std::string_view field_name) {
  StringFieldResult result = ExtractOptionalStringField(text, field_name);
  if (!result.ok) {
    return result;
  }

  if (!result.found) {
    return {false, false, {},
            "missing required field: " + std::string(field_name)};
  }

  if (result.value.empty()) {
    return {false, true, {},
            "string field must not be empty: " + std::string(field_name)};
  }

  return result;
}

/**
 * @brief Executes the skip JSON value operation.
 */
bool SkipJsonValue(std::string_view text, std::size_t* position,
                   std::string* error);

/**
 * @brief Executes the skip JSON array operation.
 */
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

/**
 * @brief Executes the skip JSON object operation.
 */
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

/**
 * @brief Executes the skip JSON primitive operation.
 */
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

/**
 * @brief Executes the skip JSON value operation.
 */
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

/**
 * @brief Executes the extract JSON object slice operation.
 */
std::optional<std::string_view> ExtractJsonObjectSlice(std::string_view text,
                                                       std::size_t position,
                                                       std::string* error) {
  if (position >= text.size() || text[position] != '{') {
    *error = "expected JSON object";
    return std::nullopt;
  }

  std::size_t end = position;
  if (!SkipJsonObject(text, &end, error)) {
    return std::nullopt;
  }

  return text.substr(position, end - position);
}

/**
 * @brief Parses grid shape at from external data.
 */
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
    int columns = 0;

    if (text[position] == '"') {
      const StringFieldResult row = ParseJsonStringAt(text, position,
                                                      field_name);
      if (!row.ok) {
        return {false, 0, 0, std::string(field_name), row.error};
      }

      columns = static_cast<int>(row.value.size());
      std::string error;
      if (!SkipJsonString(text, &position, &error)) {
        return {false, 0, 0, std::string(field_name), error};
      }
    } else {
      if (text[position] != '[') {
        return {false, 0, 0, std::string(field_name),
                "expected grid row array or string for field: " +
                    std::string(field_name)};
      }

      ++position;
      SkipWhitespace(text, &position);

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
    }

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

/**
 * @brief Parses grid shape from value from external data.
 */
GridShapeResult ParseGridShapeFromValue(std::string_view text,
                                        std::size_t value_start,
                                        std::string_view field_name) {
  if (value_start >= text.size()) {
    return {false, 0, 0, std::string(field_name),
            "missing grid value for field: " + std::string(field_name)};
  }

  if (text[value_start] == '[') {
    return ParseGridShapeAt(text, value_start, field_name);
  }

  if (text[value_start] != '{') {
    return {false, 0, 0, std::string(field_name),
            "expected grid array or grid object for field: " +
                std::string(field_name)};
  }

  std::string error;
  const std::optional<std::string_view> object =
      ExtractJsonObjectSlice(text, value_start, &error);
  if (!object.has_value()) {
    return {false, 0, 0, std::string(field_name), error};
  }

  const std::optional<std::size_t> rows_start =
      FindFieldValueStart(*object, "rows", &error);
  if (!rows_start.has_value()) {
    if (!error.empty()) {
      return {false, 0, 0, std::string(field_name), error};
    }
    return {false, 0, 0, std::string(field_name),
            "missing rows array for grid: " + std::string(field_name)};
  }

  return ParseGridShapeAt(*object, *rows_start, field_name);
}

/**
 * @brief Executes the terrain type from symbol operation.
 */
TerrainType TerrainTypeFromSymbol(char value) {
  switch (value) {
    case '.':
    case 'g':
    case 'G':
      return TerrainType::kOpenGround;
    case 'f':
    case 'F':
    case 't':
    case 'T':
      return TerrainType::kForest;
    case 'r':
    case 'R':
      return TerrainType::kRoad;
    case 's':
    case 'S':
      return TerrainType::kSwamp;
    case 'w':
    case 'W':
      return TerrainType::kWater;
    case 'u':
    case 'U':
      return TerrainType::kRuins;
    case '#':
      return TerrainType::kWall;
    default:
      return TerrainType::kUnknown;
  }
}


/**
 * @brief Executes the JSON object string array contains operation.
 */
bool JsonObjectStringArrayContains(std::string_view object,
                                   std::string_view field_name,
                                   std::string_view expected) {
  std::string error;
  const std::optional<std::size_t> array_start =
      FindFieldValueStart(object, field_name, &error);
  if (!array_start.has_value() || *array_start >= object.size() ||
      object[*array_start] != '[') {
    return false;
  }

  std::size_t position = *array_start + 1;
  SkipWhitespace(object, &position);
  while (position < object.size() && object[position] != ']') {
    if (object[position] != '"') {
      return false;
    }

    const StringFieldResult value = ParseJsonStringAt(object, position,
                                                      field_name);
    if (!value.ok) {
      return false;
    }
    if (value.value == expected) {
      return true;
    }

    if (!SkipJsonString(object, &position, &error)) {
      return false;
    }
    SkipWhitespace(object, &position);
    if (position < object.size() && object[position] == ',') {
      ++position;
      SkipWhitespace(object, &position);
    }
  }

  return false;
}

/**
 * @brief Executes the terrain type from catalog object operation.
 */
TerrainType TerrainTypeFromCatalogObject(std::string_view tile_id,
                                         std::string_view object) {
  const bool is_road = JsonObjectStringArrayContains(object, "tags", "road");
  const bool is_water = JsonObjectStringArrayContains(object, "tags", "water");
  const bool is_ruin = JsonObjectStringArrayContains(object, "tags", "ruin");
  const bool is_vegetation = JsonObjectStringArrayContains(object, "tags",
                                                           "vegetation");
  const bool is_terrain = JsonObjectStringArrayContains(object, "tags",
                                                        "terrain");
  const bool is_decor = JsonObjectStringArrayContains(object, "tags", "decor");
  const bool is_marker = JsonObjectStringArrayContains(object, "tags",
                                                       "marker");

  const StringFieldResult collision = ExtractOptionalStringField(object,
                                                                 "collision");
  const bool is_blocked = collision.ok && collision.found &&
                          collision.value == "blocked";

  if (is_road) {
    return TerrainType::kRoad;
  }
  if (is_water) {
    return TerrainType::kWater;
  }
  if (is_ruin && is_blocked) {
    return TerrainType::kWall;
  }
  if (is_ruin) {
    return TerrainType::kRuins;
  }
  if (is_vegetation) {
    return TerrainType::kForest;
  }
  if (is_terrain || is_decor || is_marker) {
    return TerrainType::kOpenGround;
  }
  if (is_blocked) {
    return TerrainType::kWall;
  }

  return TerrainTypeFromString(tile_id);
}

/**
 * @brief Parses tile catalog from external data.
 */
TileCatalog ParseTileCatalog(std::string_view text) {
  TileCatalog catalog;
  std::string error;
  const std::optional<std::size_t> types_start = FindFieldValueStart(text,
                                                                     "types",
                                                                     &error);
  if (!types_start.has_value() || *types_start >= text.size() ||
      text[*types_start] != '{') {
    return catalog;
  }

  std::size_t position = *types_start + 1;
  SkipWhitespace(text, &position);
  while (position < text.size() && text[position] != '}') {
    const StringFieldResult type_id = ParseJsonStringAt(text, position,
                                                        "types");
    if (!type_id.ok) {
      return catalog;
    }
    if (!SkipJsonString(text, &position, &error)) {
      return catalog;
    }

    SkipWhitespace(text, &position);
    if (position >= text.size() || text[position] != ':') {
      return catalog;
    }
    ++position;
    SkipWhitespace(text, &position);

    const std::optional<std::string_view> type_object =
        ExtractJsonObjectSlice(text, position, &error);
    if (!type_object.has_value()) {
      return catalog;
    }

    TileCatalogEntry entry;
    entry.terrain = TerrainTypeFromCatalogObject(type_id.value, *type_object);
    catalog.entries[type_id.value] = entry;

    if (!SkipJsonObject(text, &position, &error)) {
      return catalog;
    }
    SkipWhitespace(text, &position);
    if (position < text.size() && text[position] == ',') {
      ++position;
      SkipWhitespace(text, &position);
    }
  }

  return catalog;
}

/**
 * @brief Loads tile catalog if present.
 */
TileCatalog LoadTileCatalogIfPresent(const PackageLayout& layout) {
  if (!layout.has_tile_types_catalog) {
    return {};
  }

  std::error_code error_code;
  if (!std::filesystem::exists(layout.tile_types_catalog_path, error_code) ||
      error_code) {
    return {};
  }

  const ReadFileResult file = ReadTextFile(layout.tile_types_catalog_path);
  if (!file.ok) {
    return {};
  }

  return ParseTileCatalog(file.content);
}

/**
 * @brief Executes the terrain type from raw value operation.
 */
TerrainType TerrainTypeFromRawValue(std::string_view value,
                                    const TileCatalog* catalog) {
  if (catalog != nullptr) {
    const auto iter = catalog->entries.find(std::string(value));
    if (iter != catalog->entries.end()) {
      return iter->second.terrain;
    }
  }

  return TerrainTypeFromString(value);
}

/**
 * @brief Executes the record terrain value operation.
 */
void RecordTerrainValue(std::string_view raw_value, TerrainType terrain,
                        std::map<std::string, int>* counts,
                        std::map<std::string, int>* unknown_counts) {
  if (counts != nullptr) {
    ++(*counts)[std::string(raw_value)];
  }
  if (terrain == TerrainType::kUnknown && unknown_counts != nullptr) {
    ++(*unknown_counts)[std::string(raw_value)];
  }
}

/**
 * @brief Parses terrain grid array at from external data.
 */
TerrainGridResult ParseTerrainGridArrayAt(std::string_view text,
                                          std::size_t position,
                                          std::string_view field_name,
                                          const TileCatalog* catalog) {
  if (position >= text.size() || text[position] != '[') {
    return {false, 0, 0, {}, std::string(field_name),
            "expected terrain grid array for field: " +
                std::string(field_name)};
  }

  int rows = 0;
  int expected_columns = -1;
  std::vector<TerrainType> cells;
  std::map<std::string, int> terrain_type_counts;
  std::map<std::string, int> unknown_terrain_type_counts;
  ++position;
  SkipWhitespace(text, &position);

  if (position < text.size() && text[position] == ']') {
    return {false, 0, 0, {}, std::string(field_name),
            "terrain grid must not be empty: " + std::string(field_name)};
  }

  while (position < text.size()) {
    int columns = 0;
    std::vector<TerrainType> row_cells;

    if (text[position] == '"') {
      const StringFieldResult row = ParseJsonStringAt(text, position,
                                                      field_name);
      if (!row.ok) {
        return {false, 0, 0, {}, std::string(field_name), row.error};
      }

      row_cells.reserve(row.value.size());
      for (const char terrain_symbol : row.value) {
        const std::string raw_value(1, terrain_symbol);
        const TerrainType terrain = TerrainTypeFromSymbol(terrain_symbol);
        RecordTerrainValue(raw_value, terrain, &terrain_type_counts,
                           &unknown_terrain_type_counts);
        row_cells.push_back(terrain);
      }
      columns = static_cast<int>(row_cells.size());

      std::string error;
      if (!SkipJsonString(text, &position, &error)) {
        return {false, 0, 0, {}, std::string(field_name), error};
      }
    } else {
      if (text[position] != '[') {
        return {false, 0, 0, {}, std::string(field_name),
                "expected terrain row array or string for field: " +
                    std::string(field_name)};
      }

      ++position;
      SkipWhitespace(text, &position);

      if (position < text.size() && text[position] != ']') {
        while (position < text.size()) {
          if (text[position] != '"') {
            return {false, 0, 0, {}, std::string(field_name),
                    "terrain row values must be strings: " +
                        std::string(field_name)};
          }

          const StringFieldResult terrain = ParseJsonStringAt(text, position,
                                                              field_name);
          if (!terrain.ok) {
            return {false, 0, 0, {}, std::string(field_name), terrain.error};
          }
          const TerrainType terrain_type = TerrainTypeFromRawValue(
              terrain.value, catalog);
          RecordTerrainValue(terrain.value, terrain_type, &terrain_type_counts,
                             &unknown_terrain_type_counts);
          row_cells.push_back(terrain_type);

          std::string error;
          if (!SkipJsonString(text, &position, &error)) {
            return {false, 0, 0, {}, std::string(field_name), error};
          }

          ++columns;
          SkipWhitespace(text, &position);

          if (position >= text.size()) {
            return {false, 0, 0, {}, std::string(field_name),
                    "unterminated terrain row: " + std::string(field_name)};
          }

          if (text[position] == ',') {
            ++position;
            SkipWhitespace(text, &position);
            continue;
          }

          if (text[position] == ']') {
            break;
          }

          return {false, 0, 0, {}, std::string(field_name),
                  "expected comma or terrain row close bracket: " +
                      std::string(field_name)};
        }
      }

      if (position >= text.size() || text[position] != ']') {
        return {false, 0, 0, {}, std::string(field_name),
                "unterminated terrain row: " + std::string(field_name)};
      }
      ++position;
    }

    if (columns <= 0) {
      return {false, 0, 0, {}, std::string(field_name),
              "terrain row must not be empty: " + std::string(field_name)};
    }

    if (expected_columns < 0) {
      expected_columns = columns;
    } else if (columns != expected_columns) {
      return {false, 0, 0, {}, std::string(field_name),
              "terrain rows have different widths: " +
                  std::string(field_name)};
    }

    cells.insert(cells.end(), row_cells.begin(), row_cells.end());
    ++rows;
    SkipWhitespace(text, &position);

    if (position >= text.size()) {
      return {false, 0, 0, {}, std::string(field_name),
              "unterminated terrain grid array: " + std::string(field_name)};
    }

    if (text[position] == ',') {
      ++position;
      SkipWhitespace(text, &position);
      continue;
    }

    if (text[position] == ']') {
      ++position;
      TerrainGridResult result;
      result.ok = true;
      result.rows = rows;
      result.columns = expected_columns;
      result.cells = std::move(cells);
      result.field_name = std::string(field_name);
      result.terrain_type_counts = std::move(terrain_type_counts);
      result.unknown_terrain_type_counts = std::move(unknown_terrain_type_counts);
      result.catalog_type_count = catalog == nullptr ? 0 :
          static_cast<int>(catalog->entries.size());
      result.used_catalog = catalog != nullptr && !catalog->empty();
      return result;
    }

    return {false, 0, 0, {}, std::string(field_name),
            "expected comma or terrain grid close bracket: " +
                std::string(field_name)};
  }

  return {false, 0, 0, {}, std::string(field_name),
          "unterminated terrain grid array: " + std::string(field_name)};
}

/**
 * @brief Parses terrain grid from value from external data.
 */
TerrainGridResult ParseTerrainGridFromValue(std::string_view text,
                                            std::size_t value_start,
                                            std::string_view field_name,
                                            const TileCatalog* catalog) {
  if (value_start >= text.size()) {
    return {false, 0, 0, {}, std::string(field_name),
            "missing terrain grid value for field: " +
                std::string(field_name)};
  }

  if (text[value_start] == '[') {
    return ParseTerrainGridArrayAt(text, value_start, field_name, catalog);
  }

  if (text[value_start] != '{') {
    return {false, 0, 0, {}, std::string(field_name),
            "expected terrain grid array or object for field: " +
                std::string(field_name)};
  }

  std::string error;
  const std::optional<std::string_view> object =
      ExtractJsonObjectSlice(text, value_start, &error);
  if (!object.has_value()) {
    return {false, 0, 0, {}, std::string(field_name), error};
  }

  const std::optional<std::size_t> rows_start =
      FindFieldValueStart(*object, "rows", &error);
  if (!rows_start.has_value()) {
    if (!error.empty()) {
      return {false, 0, 0, {}, std::string(field_name), error};
    }
    return {false, 0, 0, {}, std::string(field_name),
            "missing rows array for terrain grid: " +
                std::string(field_name)};
  }

  return ParseTerrainGridArrayAt(*object, *rows_start, field_name, catalog);
}

/**
 * @brief Returns extract terrain grid.
 */
TerrainGridResult ExtractTerrainGrid(
    std::string_view text, const std::vector<std::string_view>& field_names,
    const TileCatalog* catalog) {
  std::string field_error;
  for (const std::string_view field_name : field_names) {
    const std::optional<std::size_t> value_start =
        FindFieldValueStart(text, field_name, &field_error);
    if (!value_start.has_value()) {
      if (!field_error.empty()) {
        return {false, 0, 0, {}, std::string(field_name), field_error};
      }
      continue;
    }

    return ParseTerrainGridFromValue(text, *value_start, field_name, catalog);
  }

  return {false, 0, 0, {}, {}, "missing required terrain grid field"};
}

/**
 * @brief Validates terrain grid and reports failures.
 */
bool ValidateTerrainGrid(const TerrainGridResult& grid, int expected_width,
                         int expected_height, std::string* error) {
  if (!grid.ok) {
    *error = grid.error;
    return false;
  }

  if (grid.columns != expected_width || grid.rows != expected_height) {
    *error = "terrain grid size mismatch for " + grid.field_name +
             ": expected=" + std::to_string(expected_width) + "x" +
             std::to_string(expected_height) + " actual=" +
             std::to_string(grid.columns) + "x" + std::to_string(grid.rows);
    return false;
  }

  return true;
}


/**
 * @brief Executes the extract grid shape operation.
 */
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

    return ParseGridShapeFromValue(text, *value_start, field_name);
  }

  return {false, 0, 0, {}, "missing required grid field"};
}

/**
 * @brief Validates grid shape and reports failures.
 */
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


/**
 * @brief Parses numeric value at from external data.
 */
bool ParseNumericValueAt(std::string_view text, std::size_t* position,
                         double* value, bool* present,
                         std::string* error) {
  SkipWhitespace(text, position);
  if (*position >= text.size()) {
    *error = "expected numeric grid value";
    return false;
  }

  if (text.compare(*position, 4, "null") == 0) {
    *position += 4;
    *value = 0.0;
    *present = false;
    return true;
  }
  if (text.compare(*position, 4, "true") == 0) {
    *position += 4;
    *value = 1.0;
    *present = true;
    return true;
  }
  if (text.compare(*position, 5, "false") == 0) {
    *position += 5;
    *value = 0.0;
    *present = true;
    return true;
  }

  if (text[*position] == '"') {
    const StringFieldResult string_value = ParseJsonStringAt(text, *position,
                                                             "grid_value");
    if (!string_value.ok) {
      *error = string_value.error;
      return false;
    }
    if (!SkipJsonString(text, position, error)) {
      return false;
    }
    if (string_value.value.empty()) {
      *error = "empty numeric grid string value";
      return false;
    }

    try {
      std::size_t parsed = 0;
      *value = std::stod(string_value.value, &parsed);
      if (parsed != string_value.value.size()) {
        *error = "invalid numeric grid string value";
        return false;
      }
      *present = true;
      return true;
    } catch (...) {
      *error = "invalid numeric grid string value";
      return false;
    }
  }

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
    *error = "expected numeric grid primitive";
    return false;
  }

  const std::string token(text.substr(start, *position - start));
  try {
    std::size_t parsed = 0;
    *value = std::stod(token, &parsed);
    if (parsed != token.size()) {
      *error = "invalid numeric grid primitive";
      return false;
    }
  } catch (...) {
    *error = "invalid numeric grid primitive";
    return false;
  }

  *present = true;
  return true;
}

/**
 * @brief Parses numeric grid array at from external data.
 */
NumericGridResult ParseNumericGridArrayAt(std::string_view text,
                                          std::size_t position,
                                          std::string_view field_name) {
  if (position >= text.size() || text[position] != '[') {
    return {false, 0, 0, {}, {}, std::string(field_name),
            "expected numeric grid array for field: " +
                std::string(field_name)};
  }

  int rows = 0;
  int expected_columns = -1;
  std::vector<double> values;
  std::vector<std::uint8_t> present;
  ++position;
  SkipWhitespace(text, &position);

  if (position < text.size() && text[position] == ']') {
    return {false, 0, 0, {}, {}, std::string(field_name),
            "numeric grid must not be empty: " + std::string(field_name)};
  }

  while (position < text.size()) {
    int columns = 0;
    std::vector<double> row_values;
    std::vector<std::uint8_t> row_present;

    if (text[position] == '"') {
      const StringFieldResult row = ParseJsonStringAt(text, position,
                                                      field_name);
      if (!row.ok) {
        return {false, 0, 0, {}, {}, std::string(field_name), row.error};
      }
      for (const char ch : row.value) {
        if (ch < '0' || ch > '9') {
          return {false, 0, 0, {}, {}, std::string(field_name),
                  "numeric grid string rows may contain digits only: " +
                      std::string(field_name)};
        }
        row_values.push_back(static_cast<double>(ch - '0'));
        row_present.push_back(1);
      }
      columns = static_cast<int>(row_values.size());

      std::string error;
      if (!SkipJsonString(text, &position, &error)) {
        return {false, 0, 0, {}, {}, std::string(field_name), error};
      }
    } else {
      if (text[position] != '[') {
        return {false, 0, 0, {}, {}, std::string(field_name),
                "expected numeric row array or string for field: " +
                    std::string(field_name)};
      }

      ++position;
      SkipWhitespace(text, &position);
      while (position < text.size() && text[position] != ']') {
        double value = 0.0;
        bool has_value = false;
        std::string error;
        if (!ParseNumericValueAt(text, &position, &value, &has_value,
                                 &error)) {
          return {false, 0, 0, {}, {}, std::string(field_name), error};
        }
        row_values.push_back(value);
        row_present.push_back(has_value ? 1 : 0);
        ++columns;

        SkipWhitespace(text, &position);
        if (position < text.size() && text[position] == ',') {
          ++position;
          SkipWhitespace(text, &position);
          continue;
        }
      }

      if (position >= text.size() || text[position] != ']') {
        return {false, 0, 0, {}, {}, std::string(field_name),
                "unterminated numeric grid row: " +
                    std::string(field_name)};
      }
      ++position;
    }

    if (columns <= 0) {
      return {false, 0, 0, {}, {}, std::string(field_name),
              "numeric grid row must not be empty: " +
                  std::string(field_name)};
    }

    if (expected_columns < 0) {
      expected_columns = columns;
    } else if (columns != expected_columns) {
      return {false, 0, 0, {}, {}, std::string(field_name),
              "numeric grid rows have different widths: " +
                  std::string(field_name)};
    }

    values.insert(values.end(), row_values.begin(), row_values.end());
    present.insert(present.end(), row_present.begin(), row_present.end());
    ++rows;

    SkipWhitespace(text, &position);
    if (position >= text.size()) {
      return {false, 0, 0, {}, {}, std::string(field_name),
              "unterminated numeric grid array: " +
                  std::string(field_name)};
    }
    if (text[position] == ',') {
      ++position;
      SkipWhitespace(text, &position);
      continue;
    }
    if (text[position] == ']') {
      NumericGridResult result;
      result.ok = true;
      result.rows = rows;
      result.columns = expected_columns;
      result.values = std::move(values);
      result.present = std::move(present);
      result.field_name = std::string(field_name);
      return result;
    }

    return {false, 0, 0, {}, {}, std::string(field_name),
            "expected comma or numeric grid close bracket: " +
                std::string(field_name)};
  }

  return {false, 0, 0, {}, {}, std::string(field_name),
          "unterminated numeric grid array: " + std::string(field_name)};
}

/**
 * @brief Parses numeric grid from value from external data.
 */
NumericGridResult ParseNumericGridFromValue(std::string_view text,
                                            std::size_t value_start,
                                            std::string_view field_name) {
  if (value_start >= text.size()) {
    return {false, 0, 0, {}, {}, std::string(field_name),
            "missing numeric grid value for field: " +
                std::string(field_name)};
  }

  if (text[value_start] == '[') {
    return ParseNumericGridArrayAt(text, value_start, field_name);
  }
  if (text[value_start] != '{') {
    return {false, 0, 0, {}, {}, std::string(field_name),
            "expected numeric grid array or object for field: " +
                std::string(field_name)};
  }

  std::string error;
  const std::optional<std::string_view> object =
      ExtractJsonObjectSlice(text, value_start, &error);
  if (!object.has_value()) {
    return {false, 0, 0, {}, {}, std::string(field_name), error};
  }

  const std::optional<std::size_t> rows_start =
      FindFieldValueStart(*object, "rows", &error);
  if (!rows_start.has_value()) {
    return {false, 0, 0, {}, {}, std::string(field_name),
            error.empty() ? "missing rows array for grid: " +
                                std::string(field_name)
                          : error};
  }

  return ParseNumericGridArrayAt(*object, *rows_start, field_name);
}

/**
 * @brief Returns extract numeric grid.
 */
NumericGridResult ExtractNumericGrid(
    std::string_view text, const std::vector<std::string_view>& field_names) {
  std::string field_error;
  for (const std::string_view field_name : field_names) {
    const std::optional<std::size_t> value_start =
        FindFieldValueStart(text, field_name, &field_error);
    if (!value_start.has_value()) {
      if (!field_error.empty()) {
        return {false, 0, 0, {}, {}, std::string(field_name), field_error};
      }
      continue;
    }

    return ParseNumericGridFromValue(text, *value_start, field_name);
  }

  return {false, 0, 0, {}, {}, {}, "missing required numeric grid field"};
}

/**
 * @brief Validates numeric grid and reports failures.
 */
bool ValidateNumericGrid(const NumericGridResult& grid, int expected_width,
                         int expected_height, std::string* error) {
  if (!grid.ok) {
    *error = grid.error;
    return false;
  }
  if (grid.columns != expected_width || grid.rows != expected_height) {
    *error = "numeric grid size mismatch for " + grid.field_name +
             ": expected=" + std::to_string(expected_width) + "x" +
             std::to_string(expected_height) + " actual=" +
             std::to_string(grid.columns) + "x" +
             std::to_string(grid.rows);
    return false;
  }
  return true;
}

/**
 * @brief Executes the extract object array at operation.
 */
std::optional<std::vector<std::string_view>> ExtractObjectArrayAt(
    std::string_view text, std::size_t position, std::string_view field_name,
    std::string* error) {
  if (position >= text.size() || text[position] != '[') {
    *error = "expected object array for field: " + std::string(field_name);
    return std::nullopt;
  }

  std::vector<std::string_view> objects;
  ++position;
  SkipWhitespace(text, &position);
  if (position < text.size() && text[position] == ']') {
    return objects;
  }

  while (position < text.size()) {
    if (text[position] != '{') {
      *error = "expected marker object in field: " + std::string(field_name);
      return std::nullopt;
    }

    const std::size_t object_start = position;
    if (!SkipJsonObject(text, &position, error)) {
      return std::nullopt;
    }
    objects.push_back(text.substr(object_start, position - object_start));

    SkipWhitespace(text, &position);
    if (position >= text.size()) {
      *error = "unterminated object array for field: " + std::string(field_name);
      return std::nullopt;
    }

    if (text[position] == ',') {
      ++position;
      SkipWhitespace(text, &position);
      continue;
    }

    if (text[position] == ']') {
      ++position;
      return objects;
    }

    *error = "expected comma or object array close bracket for field: " +
             std::string(field_name);
    return std::nullopt;
  }

  *error = "unterminated object array for field: " + std::string(field_name);
  return std::nullopt;
}


/**
 * @brief Executes the extract object field operation.
 */
std::optional<std::string_view> ExtractObjectField(
    std::string_view object, std::string_view field_name,
    std::string* error) {
  const std::optional<std::size_t> value_start =
      FindFieldValueStart(object, field_name, error);
  if (!value_start.has_value()) {
    return std::nullopt;
  }
  return ExtractJsonObjectSlice(object, *value_start, error);
}

/**
 * @brief Executes the extract named object array operation.
 */
std::optional<std::vector<std::string_view>> ExtractNamedObjectArray(
    std::string_view text, const std::vector<std::string_view>& field_names,
    std::string* error) {
  for (const std::string_view field_name : field_names) {
    const std::optional<std::size_t> value_start =
        FindFieldValueStart(text, field_name, error);
    if (!value_start.has_value()) {
      if (!error->empty()) {
        return std::nullopt;
      }
      continue;
    }
    return ExtractObjectArrayAt(text, *value_start, field_name, error);
  }

  std::size_t position = 0;
  SkipWhitespace(text, &position);
  if (position < text.size() && text[position] == '[') {
    return ExtractObjectArrayAt(text, position, "items", error);
  }
  return std::vector<std::string_view>{};
}

/**
 * @brief Executes the extract string array field operation.
 */
std::optional<std::vector<std::string>> ExtractStringArrayField(
    std::string_view object, std::string_view field_name) {
  std::string error;
  const std::optional<std::size_t> array_start =
      FindFieldValueStart(object, field_name, &error);
  if (!array_start.has_value() || *array_start >= object.size() ||
      object[*array_start] != '[') {
    return std::nullopt;
  }

  std::vector<std::string> values;
  std::size_t position = *array_start + 1;
  SkipWhitespace(object, &position);
  while (position < object.size() && object[position] != ']') {
    if (object[position] != '"') {
      return std::nullopt;
    }
    const StringFieldResult value = ParseJsonStringAt(object, position,
                                                      field_name);
    if (!value.ok) {
      return std::nullopt;
    }
    values.push_back(value.value);
    if (!SkipJsonString(object, &position, &error)) {
      return std::nullopt;
    }
    SkipWhitespace(object, &position);
    if (position < object.size() && object[position] == ',') {
      ++position;
      SkipWhitespace(object, &position);
    }
  }

  if (position >= object.size() || object[position] != ']') {
    return std::nullopt;
  }
  return values;
}

/**
 * @brief Stores bool field result data shared between runtime systems.
 */
struct BoolFieldResult {
  bool ok = false;
  bool found = false;
  bool value = false;
  std::string error;
};

/**
 * @brief Executes the extract optional bool field operation.
 */
BoolFieldResult ExtractOptionalBoolField(std::string_view text,
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

  if (text.compare(*value_start, 4, "true") == 0) {
    return {true, true, true, {}};
  }
  if (text.compare(*value_start, 5, "false") == 0) {
    return {true, true, false, {}};
  }
  return {false, true, false,
          "expected boolean value for field: " + std::string(field_name)};
}

/**
 * @brief Executes the extract coordinate operation.
 */
bool ExtractCoordinate(std::string_view object, int* x, int* y,
                       std::string* error) {
  IntFieldResult x_value = ExtractOptionalIntField(object, "x");
  IntFieldResult y_value = ExtractOptionalIntField(object, "y");
  if (!x_value.ok || !y_value.ok) {
    *error = !x_value.ok ? x_value.error : y_value.error;
    return false;
  }
  if (x_value.found && y_value.found) {
    *x = x_value.value;
    *y = y_value.value;
    return true;
  }

  const std::vector<std::string_view> coordinate_objects = {
      "position", "center", "anchor"};
  for (const std::string_view field_name : coordinate_objects) {
    const std::optional<std::string_view> nested =
        ExtractObjectField(object, field_name, error);
    if (!nested.has_value()) {
      if (!error->empty()) {
        return false;
      }
      continue;
    }
    x_value = ExtractRequiredIntField(*nested, "x");
    y_value = ExtractRequiredIntField(*nested, "y");
    if (!x_value.ok || !y_value.ok) {
      *error = !x_value.ok ? x_value.error : y_value.error;
      return false;
    }
    *x = x_value.value;
    *y = y_value.value;
    return true;
  }

  *error = "missing coordinate fields";
  return false;
}

/**
 * @brief Checks whether coordinate inside is true.
 */
bool IsCoordinateInside(int x, int y, int width, int height) {
  return x >= 0 && y >= 0 && x < width && y < height;
}

/**
 * @brief Checks whether footprint inside is true.
 */
bool IsFootprintInside(int x, int y, int footprint_width,
                       int footprint_height, int map_width, int map_height) {
  return x >= 0 && y >= 0 && footprint_width > 0 && footprint_height > 0 &&
         x + footprint_width <= map_width && y + footprint_height <= map_height;
}

/**
 * @brief Loads optional file.
 */
bool LoadOptionalFile(const std::filesystem::path& path,
                      ReadFileResult* file, std::string* error) {
  std::error_code error_code;
  if (!std::filesystem::exists(path, error_code)) {
    if (error_code) {
      *error = "failed to inspect semantic file: " + path.string() +
               " reason=" + error_code.message();
      return false;
    }
    file->ok = false;
    return true;
  }
  if (!std::filesystem::is_regular_file(path, error_code)) {
    if (error_code) {
      *error = "failed to inspect semantic file: " + path.string() +
               " reason=" + error_code.message();
      return false;
    }
    *error = "semantic file path is not a regular file: " + path.string();
    return false;
  }
  *file = ReadTextFile(path);
  if (!file->ok) {
    *error = file->error;
    return false;
  }
  return true;
}

/**
 * @brief Parses runtime objects from external data.
 */
RuntimeObjectLoadResult ParseRuntimeObjects(std::string_view text, int width,
                                            int height) {
  std::string error;
  const std::optional<std::vector<std::string_view>> object_values =
      ExtractNamedObjectArray(text, {"items", "objects"}, &error);
  if (!object_values.has_value()) {
    return {false, {}, error};
  }

  std::vector<RuntimeObject> objects;
  objects.reserve(object_values->size());
  for (const std::string_view object_value : *object_values) {
    const StringFieldResult id = ExtractRequiredStringField(object_value, "id");
    const StringFieldResult type = ExtractRequiredStringField(object_value,
                                                             "type");
    if (!id.ok || !type.ok) {
      return {false, {}, !id.ok ? id.error : type.error};
    }

    int x = 0;
    int y = 0;
    if (!ExtractCoordinate(object_value, &x, &y, &error)) {
      return {false, {}, "runtime object " + id.value + ": " + error};
    }

    int object_width = 1;
    int object_height = 1;
    const std::optional<std::string_view> visual_bounds =
        ExtractObjectField(object_value, "visual_bounds", &error);
    if (visual_bounds.has_value()) {
      const IntFieldResult bounds_x = ExtractOptionalIntField(*visual_bounds,
                                                              "x");
      const IntFieldResult bounds_y = ExtractOptionalIntField(*visual_bounds,
                                                              "y");
      const IntFieldResult bounds_width = ExtractOptionalIntField(
          *visual_bounds, "width");
      const IntFieldResult bounds_height = ExtractOptionalIntField(
          *visual_bounds, "height");
      if (!bounds_x.ok || !bounds_y.ok || !bounds_width.ok ||
          !bounds_height.ok) {
        return {false, {}, "invalid visual_bounds for object: " + id.value};
      }
      if (bounds_x.found) {
        x = bounds_x.value;
      }
      if (bounds_y.found) {
        y = bounds_y.value;
      }
      if (bounds_width.found && bounds_width.value > 0) {
        object_width = bounds_width.value;
      }
      if (bounds_height.found && bounds_height.value > 0) {
        object_height = bounds_height.value;
      }
    }

    if (!IsFootprintInside(x, y, object_width, object_height, width, height)) {
      return {false, {}, "runtime object is outside map bounds: " + id.value};
    }

    RuntimeObject object;
    object.id = id.value;
    object.type = type.value;
    object.x = x;
    object.y = y;
    object.width = object_width;
    object.height = object_height;

    const StringFieldResult family = ExtractOptionalStringField(object_value,
                                                                "family");
    if (!family.ok) {
      return {false, {}, family.error};
    }
    const StringFieldResult role = ExtractOptionalStringField(object_value,
                                                              "role");
    if (!role.ok) {
      return {false, {}, role.error};
    }
    object.family = family.found ? family.value :
                    role.found ? role.value : object.type;

    const IntFieldResult rotation = ExtractOptionalIntField(object_value,
                                                            "rotation");
    if (!rotation.ok) {
      return {false, {}, rotation.error};
    }
    object.rotation = rotation.found ? rotation.value : 0;

    const IntFieldResult elevation = ExtractOptionalIntField(object_value,
                                                             "elevation");
    if (!elevation.ok) {
      return {false, {}, elevation.error};
    }
    object.elevation = static_cast<std::int8_t>(
        elevation.found ? elevation.value : 0);

    const BoolFieldResult blocks_movement = ExtractOptionalBoolField(
        object_value, "blocks_movement");
    const BoolFieldResult blocks_projectiles = ExtractOptionalBoolField(
        object_value, "blocks_projectiles");
    const BoolFieldResult blocks_vision = ExtractOptionalBoolField(
        object_value, "blocks_vision");
    if (!blocks_movement.ok || !blocks_projectiles.ok || !blocks_vision.ok) {
      return {false, {}, "invalid object blocking flags: " + id.value};
    }
    object.blocks_movement = blocks_movement.found && blocks_movement.value;
    object.blocks_projectiles = blocks_projectiles.found &&
                                blocks_projectiles.value;
    object.blocks_vision = blocks_vision.found && blocks_vision.value;

    const std::optional<std::vector<std::string>> tags =
        ExtractStringArrayField(object_value, "tags");
    if (tags.has_value()) {
      object.tags = *tags;
    }
    objects.push_back(std::move(object));
  }

  return {true, std::move(objects), {}};
}

/**
 * @brief Loads runtime objects if present.
 */
RuntimeObjectLoadResult LoadRuntimeObjectsIfPresent(
    const std::filesystem::path& path, int width, int height) {
  ReadFileResult file;
  std::string error;
  if (!LoadOptionalFile(path, &file, &error)) {
    return {false, {}, error};
  }
  if (!file.ok) {
    return {true, {}, {}};
  }
  return ParseRuntimeObjects(file.content, width, height);
}

/**
 * @brief Parses places from external data.
 */
PlaceLoadResult ParsePlaces(std::string_view text, int width, int height) {
  std::string error;
  const std::optional<std::vector<std::string_view>> place_values =
      ExtractNamedObjectArray(text, {"items", "places"}, &error);
  if (!place_values.has_value()) {
    return {false, {}, error};
  }

  std::vector<Place> places;
  places.reserve(place_values->size());
  for (const std::string_view place_value : *place_values) {
    const StringFieldResult id = ExtractRequiredStringField(place_value, "id");
    const StringFieldResult type = ExtractRequiredStringField(place_value,
                                                             "type");
    if (!id.ok || !type.ok) {
      return {false, {}, !id.ok ? id.error : type.error};
    }

    int x = 0;
    int y = 0;
    if (!ExtractCoordinate(place_value, &x, &y, &error)) {
      return {false, {}, "place " + id.value + ": " + error};
    }
    if (!IsCoordinateInside(x, y, width, height)) {
      return {false, {}, "place is outside map bounds: " + id.value};
    }

    Place place;
    place.id = id.value;
    place.type = type.value;
    place.x = x;
    place.y = y;

    const StringFieldResult name = ExtractOptionalStringField(place_value,
                                                              "name");
    if (!name.ok) {
      return {false, {}, name.error};
    }
    place.name = name.found ? name.value : place.id;

    const IntFieldResult radius = ExtractOptionalIntField(place_value,
                                                          "radius");
    if (!radius.ok) {
      return {false, {}, radius.error};
    }
    place.radius = radius.found && radius.value > 0 ? radius.value : 0;

    const std::optional<std::vector<std::string>> tags =
        ExtractStringArrayField(place_value, "tags");
    if (tags.has_value()) {
      place.tags = *tags;
    }
    places.push_back(std::move(place));
  }

  return {true, std::move(places), {}};
}

/**
 * @brief Loads places if present.
 */
PlaceLoadResult LoadPlacesIfPresent(const std::filesystem::path& path,
                                    int width, int height) {
  ReadFileResult file;
  std::string error;
  if (!LoadOptionalFile(path, &file, &error)) {
    return {false, {}, error};
  }
  if (!file.ok) {
    return {true, {}, {}};
  }
  return ParsePlaces(file.content, width, height);
}

/**
 * @brief Parses routes from external data.
 */
RouteLoadResult ParseRoutes(std::string_view text, int width, int height) {
  std::string error;
  const std::optional<std::vector<std::string_view>> route_values =
      ExtractNamedObjectArray(text, {"items", "routes"}, &error);
  if (!route_values.has_value()) {
    return {false, {}, error};
  }

  std::vector<Route> routes;
  routes.reserve(route_values->size());
  for (const std::string_view route_value : *route_values) {
    const StringFieldResult id = ExtractRequiredStringField(route_value, "id");
    const StringFieldResult type = ExtractRequiredStringField(route_value,
                                                             "type");
    if (!id.ok || !type.ok) {
      return {false, {}, !id.ok ? id.error : type.error};
    }

    const std::optional<std::vector<std::string_view>> waypoint_values =
        ExtractNamedObjectArray(route_value, {"waypoints", "points"}, &error);
    if (!waypoint_values.has_value()) {
      return {false, {}, "route " + id.value + ": " + error};
    }

    Route route;
    route.id = id.value;
    route.type = type.value;
    route.waypoints.reserve(waypoint_values->size());
    for (const std::string_view waypoint_value : *waypoint_values) {
      int x = 0;
      int y = 0;
      if (!ExtractCoordinate(waypoint_value, &x, &y, &error)) {
        return {false, {}, "route waypoint " + id.value + ": " + error};
      }
      if (!IsCoordinateInside(x, y, width, height)) {
        return {false, {}, "route waypoint outside map bounds: " + id.value};
      }
      route.waypoints.push_back(RoutePoint{x, y});
    }

    const std::optional<std::vector<std::string>> tags =
        ExtractStringArrayField(route_value, "tags");
    if (tags.has_value()) {
      route.tags = *tags;
    }
    routes.push_back(std::move(route));
  }

  return {true, std::move(routes), {}};
}

/**
 * @brief Loads routes if present.
 */
RouteLoadResult LoadRoutesIfPresent(const std::filesystem::path& path,
                                    int width, int height) {
  ReadFileResult file;
  std::string error;
  if (!LoadOptionalFile(path, &file, &error)) {
    return {false, {}, error};
  }
  if (!file.ok) {
    return {true, {}, {}};
  }
  return ParseRoutes(file.content, width, height);
}

/**
 * @brief Parses world graph from external data.
 */
WorldGraphLoadResult ParseWorldGraph(std::string_view text, int width,
                                     int height) {
  std::string error;
  WorldGraph graph;

  const std::optional<std::vector<std::string_view>> node_values =
      ExtractNamedObjectArray(text, {"nodes"}, &error);
  if (!node_values.has_value()) {
    return {false, {}, error};
  }
  graph.nodes.reserve(node_values->size());
  for (const std::string_view node_value : *node_values) {
    const StringFieldResult id = ExtractRequiredStringField(node_value, "id");
    const StringFieldResult type = ExtractRequiredStringField(node_value,
                                                             "type");
    if (!id.ok || !type.ok) {
      return {false, {}, !id.ok ? id.error : type.error};
    }

    int x = 0;
    int y = 0;
    if (!ExtractCoordinate(node_value, &x, &y, &error)) {
      return {false, {}, "world graph node " + id.value + ": " + error};
    }
    if (!IsCoordinateInside(x, y, width, height)) {
      return {false, {}, "world graph node outside map bounds: " + id.value};
    }

    GraphNode node;
    node.id = id.value;
    node.type = type.value;
    node.x = x;
    node.y = y;
    graph.nodes.push_back(std::move(node));
  }

  const std::optional<std::vector<std::string_view>> edge_values =
      ExtractNamedObjectArray(text, {"edges"}, &error);
  if (!edge_values.has_value()) {
    return {false, {}, error};
  }
  graph.edges.reserve(edge_values->size());
  for (const std::string_view edge_value : *edge_values) {
    StringFieldResult from = ExtractOptionalStringField(edge_value, "from");
    if (!from.ok) {
      return {false, {}, from.error};
    }
    if (!from.found) {
      from = ExtractRequiredStringField(edge_value, "source");
    }
    StringFieldResult to = ExtractOptionalStringField(edge_value, "to");
    if (!to.ok) {
      return {false, {}, to.error};
    }
    if (!to.found) {
      to = ExtractRequiredStringField(edge_value, "target");
    }
    const StringFieldResult type = ExtractOptionalStringField(edge_value,
                                                             "type");
    if (!from.ok || !to.ok || !type.ok) {
      return {false, {}, !from.ok ? from.error : !to.ok ? to.error
                                                        : type.error};
    }

    GraphEdge edge;
    edge.from = from.value;
    edge.to = to.value;
    edge.type = type.found ? type.value : "connection";
    const IntFieldResult cost_tiles = ExtractOptionalIntField(edge_value,
                                                              "cost_tiles");
    if (!cost_tiles.ok) {
      return {false, {}, cost_tiles.error};
    }
    edge.cost = cost_tiles.found ? static_cast<float>(cost_tiles.value) : 1.0F;
    graph.edges.push_back(std::move(edge));
  }

  return {true, std::move(graph), {}};
}

/**
 * @brief Loads world graph if present.
 */
WorldGraphLoadResult LoadWorldGraphIfPresent(const std::filesystem::path& path,
                                             int width, int height) {
  ReadFileResult file;
  std::string error;
  if (!LoadOptionalFile(path, &file, &error)) {
    return {false, {}, error};
  }
  if (!file.ok) {
    return {true, {}, {}};
  }
  return ParseWorldGraph(file.content, width, height);
}

/**
 * @brief Parses gameplay zones from external data.
 */
GameplayZoneLoadResult ParseGameplayZones(std::string_view text, int width,
                                          int height) {
  std::string error;
  const std::optional<std::vector<std::string_view>> zone_values =
      ExtractNamedObjectArray(text, {"items", "zones"}, &error);
  if (!zone_values.has_value()) {
    return {false, {}, error};
  }

  std::vector<GameplayZone> zones;
  zones.reserve(zone_values->size());
  for (const std::string_view zone_value : *zone_values) {
    const StringFieldResult id = ExtractRequiredStringField(zone_value, "id");
    const StringFieldResult type = ExtractRequiredStringField(zone_value,
                                                             "type");
    if (!id.ok || !type.ok) {
      return {false, {}, !id.ok ? id.error : type.error};
    }

    GameplayZone zone;
    zone.id = id.value;
    zone.type = type.value;

    const StringFieldResult shape = ExtractOptionalStringField(zone_value,
                                                               "shape");
    if (!shape.ok) {
      return {false, {}, shape.error};
    }
    zone.shape = shape.found ? shape.value : "rect";

    const std::optional<std::string_view> bounds =
        ExtractObjectField(zone_value, "bounds", &error);
    if (bounds.has_value()) {
      const IntFieldResult min_x = ExtractRequiredIntField(*bounds, "min_x");
      const IntFieldResult min_y = ExtractRequiredIntField(*bounds, "min_y");
      const IntFieldResult max_x = ExtractRequiredIntField(*bounds, "max_x");
      const IntFieldResult max_y = ExtractRequiredIntField(*bounds, "max_y");
      if (!min_x.ok || !min_y.ok || !max_x.ok || !max_y.ok) {
        return {false, {}, "invalid zone bounds: " + id.value};
      }
      if (min_x.value < 0 || min_y.value < 0 || max_x.value >= width ||
          max_y.value >= height || max_x.value < min_x.value ||
          max_y.value < min_y.value) {
        return {false, {}, "zone bounds outside map: " + id.value};
      }
      zone.x = min_x.value;
      zone.y = min_y.value;
      zone.width = max_x.value - min_x.value + 1;
      zone.height = max_y.value - min_y.value + 1;
    } else {
      int x = 0;
      int y = 0;
      if (!ExtractCoordinate(zone_value, &x, &y, &error)) {
        return {false, {}, "zone " + id.value + ": " + error};
      }
      if (!IsCoordinateInside(x, y, width, height)) {
        return {false, {}, "zone outside map bounds: " + id.value};
      }
      zone.x = x;
      zone.y = y;
      const IntFieldResult zone_width = ExtractOptionalIntField(zone_value,
                                                                "width");
      const IntFieldResult zone_height = ExtractOptionalIntField(zone_value,
                                                                 "height");
      const IntFieldResult zone_radius = ExtractOptionalIntField(zone_value,
                                                                 "radius");
      if (!zone_width.ok || !zone_height.ok || !zone_radius.ok) {
        return {false, {}, "invalid zone dimensions: " + id.value};
      }
      zone.width = zone_width.found ? zone_width.value : 0;
      zone.height = zone_height.found ? zone_height.value : 0;
      zone.radius = zone_radius.found ? zone_radius.value : 0;
    }

    const std::optional<std::vector<std::string>> tags =
        ExtractStringArrayField(zone_value, "tags");
    if (tags.has_value()) {
      zone.tags = *tags;
    }
    zones.push_back(std::move(zone));
  }

  return {true, std::move(zones), {}};
}

/**
 * @brief Loads gameplay zones if present.
 */
GameplayZoneLoadResult LoadGameplayZonesIfPresent(
    const std::filesystem::path& path, int width, int height) {
  ReadFileResult file;
  std::string error;
  if (!LoadOptionalFile(path, &file, &error)) {
    return {false, {}, error};
  }
  if (!file.ok) {
    return {true, {}, {}};
  }
  return ParseGameplayZones(file.content, width, height);
}


/**
 * @brief Executes the extract nested coordinate operation.
 */
bool ExtractNestedCoordinate(std::string_view object,
                             std::string_view field_name,
                             int* x, int* y, std::string* error) {
  const std::optional<std::string_view> nested = ExtractObjectField(
      object, field_name, error);
  if (!nested.has_value()) {
    if (!error->empty()) {
      return false;
    }
    *error = "missing coordinate object: " + std::string(field_name);
    return false;
  }
  return ExtractCoordinate(*nested, x, y, error);
}

/**
 * @brief Executes the extract endpoint coordinate operation.
 */
bool ExtractEndpointCoordinate(std::string_view object,
                               std::string_view object_field,
                               std::string_view prefix,
                               int* x, int* y, std::string* error) {
  IntFieldResult x_value = ExtractOptionalIntField(
      object, std::string(prefix) + "_x");
  IntFieldResult y_value = ExtractOptionalIntField(
      object, std::string(prefix) + "_y");
  if (!x_value.ok || !y_value.ok) {
    *error = !x_value.ok ? x_value.error : y_value.error;
    return false;
  }
  if (x_value.found && y_value.found) {
    *x = x_value.value;
    *y = y_value.value;
    return true;
  }
  return ExtractNestedCoordinate(object, object_field, x, y, error);
}

/**
 * @brief Executes the extract optional elevation operation.
 */
std::int8_t ExtractOptionalElevation(std::string_view object,
                                     std::string_view field_name,
                                     std::int8_t fallback) {
  const IntFieldResult elevation = ExtractOptionalIntField(object, field_name);
  if (!elevation.ok || !elevation.found) {
    return fallback;
  }
  return static_cast<std::int8_t>(elevation.value);
}

/**
 * @brief Parses elevation transitions from external data.
 */
ElevationTransitionLoadResult ParseElevationTransitions(std::string_view text,
                                                        int width,
                                                        int height) {
  std::string error;
  const std::optional<std::vector<std::string_view>> transition_values =
      ExtractNamedObjectArray(text, {"items", "transitions", "edges"}, &error);
  if (!transition_values.has_value()) {
    return {false, {}, error};
  }

  std::vector<ElevationTransition> transitions;
  transitions.reserve(transition_values->size());
  int fallback_id = 0;
  for (const std::string_view transition_value : *transition_values) {
    int from_x = 0;
    int from_y = 0;
    int to_x = 0;
    int to_y = 0;
    if (!ExtractEndpointCoordinate(transition_value, "from", "from", &from_x,
                                   &from_y, &error)) {
      return {false, {}, "elevation transition: " + error};
    }
    if (!ExtractEndpointCoordinate(transition_value, "to", "to", &to_x,
                                   &to_y, &error)) {
      return {false, {}, "elevation transition: " + error};
    }
    if (!IsCoordinateInside(from_x, from_y, width, height) ||
        !IsCoordinateInside(to_x, to_y, width, height)) {
      return {false, {}, "elevation transition endpoint outside map"};
    }

    const StringFieldResult id = ExtractOptionalStringField(transition_value,
                                                            "id");
    if (!id.ok) {
      return {false, {}, id.error};
    }
    const StringFieldResult type = ExtractOptionalStringField(transition_value,
                                                              "type");
    if (!type.ok) {
      return {false, {}, type.error};
    }
    const BoolFieldResult bidirectional = ExtractOptionalBoolField(
        transition_value, "bidirectional");
    if (!bidirectional.ok) {
      return {false, {}, bidirectional.error};
    }

    ElevationTransition transition;
    if (id.found && !id.value.empty()) {
      transition.id = id.value;
    } else {
      transition.id = "elevation_transition_" + std::to_string(fallback_id);
    }
    ++fallback_id;
    transition.type = type.found ? ParseElevationTransitionType(type.value)
                                 : ElevationTransitionType::kUnknown;
    transition.from_x = from_x;
    transition.from_y = from_y;
    transition.to_x = to_x;
    transition.to_y = to_y;
    transition.from_elevation = ExtractOptionalElevation(
        transition_value, "from_elevation", 0);
    transition.to_elevation = ExtractOptionalElevation(
        transition_value, "to_elevation", transition.from_elevation);
    if (!ValidateElevationRange(transition.from_elevation,
                                "elevation transition from", &error) ||
        !ValidateElevationRange(transition.to_elevation,
                                "elevation transition to", &error)) {
      return {false, {}, "elevation transition " + transition.id + ": " +
                             error};
    }
    transition.bidirectional = !bidirectional.found || bidirectional.value;
    transitions.push_back(std::move(transition));
  }

  return {true, std::move(transitions), {}};
}

/**
 * @brief Loads elevation transitions if present.
 */
ElevationTransitionLoadResult LoadElevationTransitionsIfPresent(
    const std::filesystem::path& path, int width, int height) {
  ReadFileResult file;
  std::string error;
  if (!LoadOptionalFile(path, &file, &error)) {
    return {false, {}, error};
  }
  if (!file.ok) {
    return {true, {}, {}};
  }
  return ParseElevationTransitions(file.content, width, height);
}

/**
 * @brief Parses markers from external data.
 */
MarkerLoadResult ParseMarkers(std::string_view text, int width, int height) {
  std::string error;
  std::string_view array_field_name = "markers";
  std::optional<std::size_t> markers_start = FindFieldValueStart(text,
                                                                 array_field_name,
                                                                 &error);
  if (!markers_start.has_value()) {
    if (!error.empty()) {
      return {false, {}, error};
    }

    array_field_name = "items";
    markers_start = FindFieldValueStart(text, array_field_name, &error);
    if (!markers_start.has_value()) {
      if (!error.empty()) {
        return {false, {}, error};
      }

      std::size_t position = 0;
      SkipWhitespace(text, &position);
      if (position >= text.size() || text[position] != '[') {
        return {true, {}, {}};
      }
      markers_start = position;
      array_field_name = "markers";
    }
  }

  const std::optional<std::vector<std::string_view>> marker_objects =
      ExtractObjectArrayAt(text, *markers_start, array_field_name, &error);
  if (!marker_objects.has_value()) {
    return {false, {}, error};
  }

  std::vector<Marker> markers;
  markers.reserve(marker_objects->size());

  for (const std::string_view marker_object : *marker_objects) {
    const StringFieldResult id = ExtractRequiredStringField(marker_object,
                                                            "id");
    if (!id.ok) {
      return {false, {}, id.error};
    }

    const StringFieldResult type = ExtractRequiredStringField(marker_object,
                                                              "type");
    if (!type.ok) {
      return {false, {}, type.error};
    }

    const IntFieldResult x = ExtractRequiredIntField(marker_object, "x");
    if (!x.ok) {
      return {false, {}, x.error};
    }

    const IntFieldResult y = ExtractRequiredIntField(marker_object, "y");
    if (!y.ok) {
      return {false, {}, y.error};
    }

    if (x.value < 0 || x.value >= width || y.value < 0 || y.value >= height) {
      return {false, {}, "marker is outside map bounds: " + id.value};
    }

    const IntFieldResult elevation = ExtractOptionalIntField(marker_object,
                                                             "elevation");
    if (!elevation.ok) {
      return {false, {}, elevation.error};
    }

    Marker marker;
    marker.id = id.value;
    marker.type = type.value;
    marker.x = x.value;
    marker.y = y.value;
    marker.elevation = static_cast<std::int8_t>(
        elevation.found ? elevation.value : 0);
    markers.push_back(std::move(marker));
  }

  return {true, std::move(markers), {}};
}

/**
 * @brief Loads markers if present.
 */
MarkerLoadResult LoadMarkersIfPresent(const std::filesystem::path& path,
                                      int width, int height) {
  std::error_code error_code;
  if (!std::filesystem::exists(path, error_code)) {
    if (error_code) {
      return {false, {}, "failed to inspect markers file: " + path.string() +
                             " reason=" + error_code.message()};
    }
    return {true, {}, {}};
  }

  if (!std::filesystem::is_regular_file(path, error_code)) {
    if (error_code) {
      return {false, {}, "failed to inspect markers file: " + path.string() +
                             " reason=" + error_code.message()};
    }
    return {false, {}, "markers path is not a regular file: " +
                       path.string()};
  }

  const ReadFileResult file = ReadTextFile(path);
  if (!file.ok) {
    return {false, {}, file.error};
  }

  return ParseMarkers(file.content, width, height);
}

/**
 * @brief Executes the resolve package file operation.
 */
std::filesystem::path ResolvePackageFile(
    const std::filesystem::path& package_path,
    const std::string& relative_path) {
  return (package_path / std::filesystem::path(relative_path)).lexically_normal();
}

/**
 * @brief Executes the resolve package layout operation.
 */
LevelLoadResult ResolvePackageLayout(const std::filesystem::path& package_path,
                                     PackageLayout* layout) {
  layout->package_path = package_path;
  layout->terrain_path = package_path / "terrain.json";
  layout->runtime_grids_path = package_path / "runtime_grids.json";
  layout->markers_path = package_path / "markers.json";
  layout->runtime_objects_path = package_path / "objects" / "runtime_objects.json";
  layout->places_path = package_path / "objects" / "places.json";
  layout->routes_path = package_path / "routes.json";
  layout->world_graph_path = package_path / "world_graph.json";
  layout->gameplay_zones_path = package_path / "gameplay_zones.json";
  layout->elevation_transitions_path = package_path / "elevation_transitions.json";

  const std::filesystem::path manifest_path = package_path / "map.json";
  std::error_code error_code;
  if (!std::filesystem::exists(manifest_path, error_code)) {
    if (error_code) {
      return {false, {}, "failed to inspect map manifest: " +
                             manifest_path.string() +
                             " reason=" + error_code.message()};
    }
    return {true, {}, {}};
  }

  if (!std::filesystem::is_regular_file(manifest_path, error_code)) {
    if (error_code) {
      return {false, {}, "failed to inspect map manifest: " +
                             manifest_path.string() +
                             " reason=" + error_code.message()};
    }
    return {false, {}, "map manifest is not a regular file: " +
                           manifest_path.string()};
  }

  const ReadFileResult manifest = ReadTextFile(manifest_path);
  if (!manifest.ok) {
    return {false, {}, manifest.error};
  }

  const StringFieldResult terrain = ExtractRequiredStringField(
      manifest.content, "terrain");
  if (!terrain.ok) {
    return {false, {}, terrain.error};
  }

  const StringFieldResult runtime_grids = ExtractRequiredStringField(
      manifest.content, "runtime_grids");
  if (!runtime_grids.ok) {
    return {false, {}, runtime_grids.error};
  }

  layout->uses_manifest = true;
  layout->terrain_path = ResolvePackageFile(package_path, terrain.value);
  layout->runtime_grids_path = ResolvePackageFile(package_path,
                                                  runtime_grids.value);

  const StringFieldResult markers = ExtractOptionalStringField(
      manifest.content, "markers");
  if (!markers.ok) {
    return {false, {}, markers.error};
  }
  if (markers.found && !markers.value.empty()) {
    layout->markers_path = ResolvePackageFile(package_path, markers.value);
  }


  const StringFieldResult runtime_objects = ExtractOptionalStringField(
      manifest.content, "runtime_objects");
  if (!runtime_objects.ok) {
    return {false, {}, runtime_objects.error};
  }
  if (runtime_objects.found && !runtime_objects.value.empty()) {
    layout->runtime_objects_path = ResolvePackageFile(package_path,
                                                       runtime_objects.value);
  }

  const StringFieldResult places = ExtractOptionalStringField(manifest.content,
                                                              "places");
  if (!places.ok) {
    return {false, {}, places.error};
  }
  if (places.found && !places.value.empty()) {
    layout->places_path = ResolvePackageFile(package_path, places.value);
  }

  const StringFieldResult routes = ExtractOptionalStringField(manifest.content,
                                                              "routes");
  if (!routes.ok) {
    return {false, {}, routes.error};
  }
  if (routes.found && !routes.value.empty()) {
    layout->routes_path = ResolvePackageFile(package_path, routes.value);
  }

  const StringFieldResult world_graph = ExtractOptionalStringField(
      manifest.content, "world_graph");
  if (!world_graph.ok) {
    return {false, {}, world_graph.error};
  }
  if (world_graph.found && !world_graph.value.empty()) {
    layout->world_graph_path = ResolvePackageFile(package_path,
                                                  world_graph.value);
  }

  const StringFieldResult gameplay_zones = ExtractOptionalStringField(
      manifest.content, "gameplay_zones");
  if (!gameplay_zones.ok) {
    return {false, {}, gameplay_zones.error};
  }
  if (gameplay_zones.found && !gameplay_zones.value.empty()) {
    layout->gameplay_zones_path = ResolvePackageFile(package_path,
                                                     gameplay_zones.value);
  }

  const StringFieldResult elevation_transitions = ExtractOptionalStringField(
      manifest.content, "elevation_transitions");
  if (!elevation_transitions.ok) {
    return {false, {}, elevation_transitions.error};
  }
  if (elevation_transitions.found && !elevation_transitions.value.empty()) {
    layout->elevation_transitions_path = ResolvePackageFile(
        package_path, elevation_transitions.value);
  }

  const StringFieldResult tile_types = ExtractOptionalStringField(
      manifest.content, "tile_types");
  if (!tile_types.ok) {
    return {false, {}, tile_types.error};
  }
  if (tile_types.found && !tile_types.value.empty()) {
    layout->tile_types_catalog_path = ResolvePackageFile(package_path,
                                                         tile_types.value);
    layout->has_tile_types_catalog = true;
  }

  const IntFieldResult width_tiles = ExtractOptionalIntField(
      manifest.content, "width_tiles");
  const IntFieldResult height_tiles = ExtractOptionalIntField(
      manifest.content, "height_tiles");
  const IntFieldResult tile_size_px = ExtractOptionalIntField(
      manifest.content, "tile_size_px");
  if (!width_tiles.ok || !height_tiles.ok || !tile_size_px.ok) {
    return {false, {}, !width_tiles.ok ? width_tiles.error
                      : !height_tiles.ok ? height_tiles.error
                                         : tile_size_px.error};
  }

  if (width_tiles.found || height_tiles.found || tile_size_px.found) {
    if (!width_tiles.found || !height_tiles.found || !tile_size_px.found) {
      return {false, {}, "manifest dimensions are incomplete"};
    }

    if (width_tiles.value <= 0 || height_tiles.value <= 0 ||
        tile_size_px.value <= 0) {
      return {false, {}, "manifest dimensions must be positive"};
    }

    layout->has_manifest_size = true;
    layout->manifest_size = LevelSize{width_tiles.value, height_tiles.value,
                                      tile_size_px.value};
  }

  return {true, {}, {}};
}


constexpr int kMainlineMinElevation = -1;
constexpr int kMainlineMaxElevation = 4;

/**
 * @brief Formats an integer histogram for one-line diagnostics.
 */
std::string FormatIntHistogram(const std::map<int, int>& histogram) {
  std::ostringstream stream;
  bool first = true;
  for (const auto& [key, count] : histogram) {
    if (!first) {
      stream << ' ';
    }
    first = false;
    stream << key << '=' << count;
  }
  return stream.str();
}

/**
 * @brief Formats a string histogram for one-line diagnostics.
 */
std::string FormatStringHistogram(const std::map<std::string, int>& histogram) {
  std::ostringstream stream;
  bool first = true;
  for (const auto& [key, count] : histogram) {
    if (!first) {
      stream << ' ';
    }
    first = false;
    stream << key << '=' << count;
  }
  return stream.str();
}

/**
 * @brief Adds a validation warning with a small cap to keep startup logs readable.
 */
void AddValidationWarning(const std::string& warning,
                          LevelPackageValidationReport* report) {
  if (report == nullptr || report->warnings.size() >= 12) {
    return;
  }
  report->warnings.push_back(warning);
}

/**
 * @brief Returns the row-major cell index for a tile coordinate.
 */
std::size_t LevelCellIndex(int x, int y, int width) {
  return static_cast<std::size_t>(y * width + x);
}

/**
 * @brief Validates that a loaded elevation belongs to the active mainline range.
 */
bool ValidateElevationRange(int elevation, std::string_view source,
                            std::string* error) {
  if (elevation < kMainlineMinElevation ||
      elevation > kMainlineMaxElevation) {
    *error = std::string(source) + " elevation outside supported range " +
             std::to_string(kMainlineMinElevation) + ".." +
             std::to_string(kMainlineMaxElevation) + ": " +
             std::to_string(elevation);
    return false;
  }
  return true;
}

/**
 * @brief Builds the one-time map package validation report.
 */
LevelPackageValidationReport BuildValidationReport(const LevelData& level) {
  LevelPackageValidationReport report;
  report.total_tiles = level.size.width * level.size.height;
  report.terrain_histogram = level.terrain_type_counts;
  report.min_elevation = std::numeric_limits<int>::max();
  report.max_elevation = std::numeric_limits<int>::min();

  for (const RuntimeCell& cell : level.cells) {
    if (cell.walkable) {
      ++report.walkable_tiles;
    }
    if (cell.collision) {
      ++report.collision_tiles;
    }
    if (cell.blocks_projectiles) {
      ++report.projectile_block_tiles;
    }
    if (cell.blocks_vision) {
      ++report.vision_block_tiles;
    }
    if (cell.cover > 0) {
      ++report.cover_tiles;
    }
    if (cell.concealment > 0) {
      ++report.concealment_tiles;
    }

    const int elevation = cell.height;
    report.min_elevation = std::min(report.min_elevation, elevation);
    report.max_elevation = std::max(report.max_elevation, elevation);
    ++report.elevation_histogram[elevation];
    if (elevation < 0) {
      ++report.negative_region_tile_count;
    }
  }
  if (level.cells.empty()) {
    report.min_elevation = 0;
    report.max_elevation = 0;
  }

  for (const ElevationTransition& transition : level.elevation_transitions) {
    ++report.transition_histogram[ElevationTransitionTypeName(transition.type)];
    const int from_delta = transition.to_elevation - transition.from_elevation;
    if (std::abs(from_delta) > 1) {
      ++report.transition_large_delta_count;
    }

    if (!IsCoordinateInside(transition.from_x, transition.from_y,
                            level.size.width, level.size.height) ||
        !IsCoordinateInside(transition.to_x, transition.to_y,
                            level.size.width, level.size.height)) {
      continue;
    }

    const RuntimeCell& from_cell = level.cells[LevelCellIndex(
        transition.from_x, transition.from_y, level.size.width)];
    const RuntimeCell& to_cell = level.cells[LevelCellIndex(
        transition.to_x, transition.to_y, level.size.width)];
    if (from_cell.height != transition.from_elevation ||
        to_cell.height != transition.to_elevation) {
      ++report.transition_endpoint_mismatch_count;
      if (report.transition_endpoint_mismatch_count <= 4) {
        AddValidationWarning(
            "transition endpoint elevation mismatch id=" + transition.id +
                " declared=" + std::to_string(transition.from_elevation) +
                "->" + std::to_string(transition.to_elevation) +
                " actual=" + std::to_string(from_cell.height) + "->" +
                std::to_string(to_cell.height),
            &report);
      }
    }
  }

  for (const Marker& marker : level.markers) {
    if (!IsCoordinateInside(marker.x, marker.y, level.size.width,
                            level.size.height)) {
      continue;
    }
    const RuntimeCell& cell = level.cells[LevelCellIndex(marker.x, marker.y,
                                                        level.size.width)];
    if (marker.elevation != cell.height) {
      AddValidationWarning(
          "marker elevation mismatch id=" + marker.id +
              " declared=" + std::to_string(marker.elevation) +
              " actual=" + std::to_string(cell.height),
          &report);
    }
  }

  std::vector<std::uint8_t> visited(level.cells.size(), 0);
  std::vector<std::pair<int, int>> stack;
  constexpr int kDx[4] = {1, -1, 0, 0};
  constexpr int kDy[4] = {0, 0, 1, -1};

  for (int y = 0; y < level.size.height; ++y) {
    for (int x = 0; x < level.size.width; ++x) {
      const std::size_t start_index = LevelCellIndex(x, y, level.size.width);
      if (visited[start_index] != 0 || level.cells[start_index].height >= 0) {
        continue;
      }

      ++report.negative_region_count;
      int region_tiles = 0;
      int walkable_boundary_entries = 0;
      stack.clear();
      stack.emplace_back(x, y);
      visited[start_index] = 1;

      while (!stack.empty()) {
        const auto [current_x, current_y] = stack.back();
        stack.pop_back();
        ++region_tiles;
        const RuntimeCell& current_cell = level.cells[LevelCellIndex(
            current_x, current_y, level.size.width)];

        for (int direction = 0; direction < 4; ++direction) {
          const int next_x = current_x + kDx[direction];
          const int next_y = current_y + kDy[direction];
          if (!IsCoordinateInside(next_x, next_y, level.size.width,
                                  level.size.height)) {
            continue;
          }
          const std::size_t next_index = LevelCellIndex(next_x, next_y,
                                                        level.size.width);
          const RuntimeCell& next_cell = level.cells[next_index];
          if (next_cell.height < 0) {
            if (visited[next_index] == 0) {
              visited[next_index] = 1;
              stack.emplace_back(next_x, next_y);
            }
            continue;
          }

          if (current_cell.walkable && !current_cell.collision &&
              next_cell.walkable && !next_cell.collision) {
            ++walkable_boundary_entries;
          }
        }
      }

      if (walkable_boundary_entries > 0) {
        ++report.open_negative_region_count;
      } else {
        ++report.closed_negative_region_count;
        AddValidationWarning(
            "negative elevation region has no walkable boundary entry tiles=" +
                std::to_string(region_tiles),
            &report);
      }
    }
  }

  if (report.min_elevation < kMainlineMinElevation ||
      report.max_elevation > kMainlineMaxElevation) {
    AddValidationWarning(
        "height range is outside active mainline range " +
            std::to_string(kMainlineMinElevation) + ".." +
            std::to_string(kMainlineMaxElevation),
        &report);
  }
  if (report.transition_endpoint_mismatch_count > 4) {
    AddValidationWarning(
        "additional transition endpoint mismatches: " +
            std::to_string(report.transition_endpoint_mismatch_count - 4),
        &report);
  }
  return report;
}

}  // namespace


/**
 * @brief Builds a readable multi-line validation report for startup diagnostics.
 */
std::string LevelPackageValidationReport::DumpMultiline() const {
  std::ostringstream stream;
  stream << "Level package validation report:\n";
  stream << "  tiles: total=" << total_tiles << " walkable=" << walkable_tiles
         << " collision=" << collision_tiles
         << " projectile_block=" << projectile_block_tiles
         << " vision_block=" << vision_block_tiles << '\n';
  stream << "  gameplay: cover=" << cover_tiles
         << " concealment=" << concealment_tiles << '\n';
  stream << "  elevation: range=" << min_elevation << ".." << max_elevation
         << " supported=" << kMainlineMinElevation << ".."
         << kMainlineMaxElevation << " histogram={"
         << FormatIntHistogram(elevation_histogram) << "}\n";
  stream << "  terrain: {" << FormatStringHistogram(terrain_histogram)
         << "}\n";
  stream << "  transitions: {" << FormatStringHistogram(transition_histogram)
         << "} endpoint_mismatches=" << transition_endpoint_mismatch_count
         << " large_delta=" << transition_large_delta_count << '\n';
  stream << "  negative_regions: total=" << negative_region_count
         << " open=" << open_negative_region_count
         << " closed=" << closed_negative_region_count
         << " tiles=" << negative_region_tile_count << '\n';
  if (warnings.empty()) {
    stream << "  warnings: 0";
  } else {
    stream << "  warnings: " << warnings.size();
    for (const std::string& warning : warnings) {
      stream << "\n    - " << warning;
    }
  }
  return stream.str();
}

/**
 * @brief Builds a readable diagnostic dump for dump.
 */
std::string LevelPackageSummary::Dump() const {
  return "LevelPackageSummary { path: \"" + package_path.string() +
         "\", width: " + std::to_string(size.width) +
         ", height: " + std::to_string(size.height) +
         ", tile_size: " + std::to_string(size.tile_size) +
         ", runtime_grids: " +
         std::to_string(validated_runtime_grid_count) +
         ", markers: " + std::to_string(marker_count) +
         ", objects: " + std::to_string(object_count) +
         ", places: " + std::to_string(place_count) +
         ", routes: " + std::to_string(route_count) +
         ", elevation_transitions: " +
         std::to_string(elevation_transition_count) +
         ", zones: " + std::to_string(gameplay_zone_count) +
         ", graph_nodes: " + std::to_string(graph_node_count) +
         ", graph_edges: " + std::to_string(graph_edge_count) + " }";
}

/**
 * @brief Loads basic package.
 */
LevelLoadResult LevelLoader::LoadBasicPackage(
    const std::filesystem::path& package_path) const {
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

  PackageLayout layout;
  const LevelLoadResult layout_result = ResolvePackageLayout(package_path,
                                                             &layout);
  if (!layout_result.ok) {
    return layout_result;
  }

  const ReadFileResult terrain_file = ReadTextFile(layout.terrain_path);
  if (!terrain_file.ok) {
    return {false, {}, terrain_file.error};
  }

  const ReadFileResult runtime_grids_file = ReadTextFile(
      layout.runtime_grids_path);
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

  IntFieldResult tile_size = ExtractOptionalIntField(terrain_file.content,
                                                     "tile_size");
  if (!tile_size.ok) {
    return {false, {}, tile_size.error};
  }

  int resolved_tile_size = kDefaultTileSize;
  if (tile_size.found) {
    if (tile_size.value <= 0) {
      return {false, {}, "tile_size must be positive"};
    }
    resolved_tile_size = tile_size.value;
  } else if (layout.has_manifest_size) {
    resolved_tile_size = layout.manifest_size.tile_size;
  }

  if (layout.has_manifest_size &&
      (layout.manifest_size.width != width.value ||
       layout.manifest_size.height != height.value)) {
    return {false, {}, "manifest and terrain dimensions mismatch"};
  }

  const TileCatalog tile_catalog = LoadTileCatalogIfPresent(layout);

  const TerrainGridResult terrain_grid = ExtractTerrainGrid(
      terrain_file.content, {"terrain_grid", "terrain", "grid", "rows"},
      tile_catalog.empty() ? nullptr : &tile_catalog);
  std::string error;
  if (!ValidateTerrainGrid(terrain_grid, width.value, height.value, &error)) {
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

  const NumericGridResult movement_grid = ExtractNumericGrid(
      runtime_grids_file.content, {"movement_grid"});
  const NumericGridResult collision_grid = ExtractNumericGrid(
      runtime_grids_file.content, {"collision_grid"});
  const NumericGridResult projectile_grid = ExtractNumericGrid(
      runtime_grids_file.content, {"projectile_block_grid"});
  const NumericGridResult vision_grid = ExtractNumericGrid(
      runtime_grids_file.content, {"vision_block_grid"});
  const NumericGridResult cover_grid = ExtractNumericGrid(
      runtime_grids_file.content, {"cover_grid"});
  const NumericGridResult concealment_grid = ExtractNumericGrid(
      runtime_grids_file.content, {"concealment_grid"});
  const NumericGridResult height_grid = ExtractNumericGrid(
      runtime_grids_file.content, {"height_grid"});

  const std::vector<const NumericGridResult*> numeric_grids = {
      &movement_grid, &collision_grid, &projectile_grid, &vision_grid,
      &cover_grid, &concealment_grid, &height_grid};
  for (const NumericGridResult* grid : numeric_grids) {
    if (!ValidateNumericGrid(*grid, width.value, height.value, &error)) {
      return {false, {}, error};
    }
  }

  LevelPackageSummary summary;
  summary.package_path = package_path;
  summary.size = LevelSize{width.value, height.value, resolved_tile_size};
  summary.validated_runtime_grid_count = validated_runtime_grid_count;

  const MarkerLoadResult markers = LoadMarkersIfPresent(
      layout.markers_path, width.value, height.value);
  if (!markers.ok) {
    return {false, {}, markers.error};
  }
  for (const Marker& marker : markers.markers) {
    const std::string marker_source = "marker " + marker.id;
    if (!ValidateElevationRange(marker.elevation, marker_source, &error)) {
      return {false, {}, error};
    }
  }
  summary.marker_count = static_cast<int>(markers.markers.size());

  const RuntimeObjectLoadResult runtime_objects = LoadRuntimeObjectsIfPresent(
      layout.runtime_objects_path, width.value, height.value);
  if (!runtime_objects.ok) {
    return {false, {}, runtime_objects.error};
  }
  for (const RuntimeObject& object : runtime_objects.objects) {
    const std::string object_source = "runtime object " + object.id;
    if (!ValidateElevationRange(object.elevation, object_source, &error)) {
      return {false, {}, error};
    }
  }
  summary.object_count = static_cast<int>(runtime_objects.objects.size());

  const PlaceLoadResult places = LoadPlacesIfPresent(
      layout.places_path, width.value, height.value);
  if (!places.ok) {
    return {false, {}, places.error};
  }
  summary.place_count = static_cast<int>(places.places.size());

  const RouteLoadResult routes = LoadRoutesIfPresent(
      layout.routes_path, width.value, height.value);
  if (!routes.ok) {
    return {false, {}, routes.error};
  }
  summary.route_count = static_cast<int>(routes.routes.size());

  const ElevationTransitionLoadResult elevation_transitions =
      LoadElevationTransitionsIfPresent(layout.elevation_transitions_path,
                                        width.value, height.value);
  if (!elevation_transitions.ok) {
    return {false, {}, elevation_transitions.error};
  }
  summary.elevation_transition_count = static_cast<int>(
      elevation_transitions.transitions.size());

  const WorldGraphLoadResult world_graph = LoadWorldGraphIfPresent(
      layout.world_graph_path, width.value, height.value);
  if (!world_graph.ok) {
    return {false, {}, world_graph.error};
  }
  summary.graph_node_count = static_cast<int>(world_graph.graph.nodes.size());
  summary.graph_edge_count = static_cast<int>(world_graph.graph.edges.size());

  const GameplayZoneLoadResult gameplay_zones = LoadGameplayZonesIfPresent(
      layout.gameplay_zones_path, width.value, height.value);
  if (!gameplay_zones.ok) {
    return {false, {}, gameplay_zones.error};
  }
  summary.gameplay_zone_count =
      static_cast<int>(gameplay_zones.zones.size());

  LevelData level;
  level.size = summary.size;
  level.markers = std::move(markers.markers);
  level.objects = std::move(runtime_objects.objects);
  level.places = std::move(places.places);
  level.routes = std::move(routes.routes);
  level.elevation_transitions = std::move(elevation_transitions.transitions);
  level.world_graph = std::move(world_graph.graph);
  level.zones = std::move(gameplay_zones.zones);
  level.terrain_type_counts = terrain_grid.terrain_type_counts;
  level.unknown_terrain_type_counts = terrain_grid.unknown_terrain_type_counts;
  level.tile_catalog_type_count = terrain_grid.catalog_type_count;
  level.used_tile_catalog = terrain_grid.used_catalog;
  level.cells.reserve(terrain_grid.cells.size());

  const std::size_t cell_count = terrain_grid.cells.size();
  for (std::size_t index = 0; index < cell_count; ++index) {
    RuntimeCell cell;
    cell.terrain = terrain_grid.cells[index];
    const double movement_value = movement_grid.present[index] == 0
                                      ? 0.0
                                      : movement_grid.values[index];
    cell.walkable = movement_grid.present[index] != 0 && movement_value > 0.0;
    cell.collision = collision_grid.present[index] != 0 &&
                     collision_grid.values[index] != 0.0;
    cell.movement_multiplier = NormalizeMovementMultiplier(
        movement_value, cell.terrain, cell.walkable, cell.collision);
    cell.blocks_projectiles = projectile_grid.present[index] != 0 &&
                              projectile_grid.values[index] != 0.0;
    cell.blocks_vision = vision_grid.present[index] != 0 &&
                         vision_grid.values[index] != 0.0;
    cell.cover = static_cast<std::uint8_t>(
        cover_grid.present[index] == 0 || cover_grid.values[index] <= 0.0
            ? 0
            : 1);
    cell.concealment = static_cast<std::uint8_t>(
        concealment_grid.present[index] == 0 ||
                concealment_grid.values[index] <= 0.0
            ? 0
            : 1);
    const double raw_height = height_grid.present[index] == 0
                                  ? 0.0
                                  : height_grid.values[index];
    const int elevation = static_cast<int>(raw_height);
    if (raw_height != static_cast<double>(elevation)) {
      return {false, {}, "height_grid contains non-integer elevation at index " +
                             std::to_string(index)};
    }
    if (!ValidateElevationRange(elevation, "height_grid", &error)) {
      return {false, {}, error + " at index " + std::to_string(index)};
    }
    cell.height = static_cast<std::int8_t>(elevation);
    level.cells.push_back(cell);
  }

  summary.validation_report = BuildValidationReport(level);

  return {true, summary, {}, level};
}

}  // namespace sar
