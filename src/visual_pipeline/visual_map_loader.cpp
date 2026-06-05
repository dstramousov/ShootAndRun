#include "visual_pipeline/visual_map_loader.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace sar::visual_pipeline {
namespace {

struct ReadFileResult {
  bool ok = false;
  std::string text;
  std::string error;
};

struct ParseStringResult {
  bool ok = false;
  bool found = false;
  std::string value;
  std::string error;
};

struct ParseIntResult {
  bool ok = false;
  bool found = false;
  int value = 0;
  std::string error;
};

struct ParseBoolResult {
  bool ok = false;
  bool found = false;
  bool value = false;
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

ParseStringResult ParseJsonString(std::string_view text,
                                  std::size_t position) {
  if (position >= text.size() || text[position] != '"') {
    return {false, true, {}, "expected string value"};
  }

  std::string value;
  ++position;
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
      return {false, true, {}, "unterminated escape sequence"};
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
        return {false, true, {}, "unsupported escape sequence"};
    }
  }

  return {false, true, {}, "unterminated string value"};
}

ParseStringResult ExtractOptionalJsonStringField(
    std::string_view text, std::string_view field_name) {
  std::string error;
  const std::optional<std::size_t> value_start =
      FindFieldValueStart(text, field_name, &error);
  if (!value_start.has_value()) {
    if (!error.empty()) {
      return {false, false, {}, error};
    }
    return {true, false, {}, {}};
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
  for (std::size_t i = *value_start; i < end; ++i) {
    if (text[i] == '-') {
      continue;
    }
    value = value * 10 + static_cast<int>(text[i] - '0');
  }
  if (text[*value_start] == '-') {
    value = -value;
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

std::optional<std::string_view> ExtractJsonComposite(
    std::string_view text, std::string_view field_name, char open_char,
    char close_char, std::string* error) {
  const std::optional<std::size_t> value_start =
      FindFieldValueStart(text, field_name, error);
  if (!value_start.has_value()) {
    return std::nullopt;
  }
  if (*value_start >= text.size() || text[*value_start] != open_char) {
    *error = "expected composite value for field: " + std::string(field_name);
    return std::nullopt;
  }

  int depth = 0;
  bool in_string = false;
  bool escaped = false;
  for (std::size_t position = *value_start; position < text.size();
       ++position) {
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
    if (current == open_char) {
      ++depth;
    } else if (current == close_char) {
      --depth;
      if (depth == 0) {
        return text.substr(*value_start, position - *value_start + 1);
      }
    }
  }

  *error = "unterminated composite field: " + std::string(field_name);
  return std::nullopt;
}

std::optional<std::string_view> ExtractJsonObject(
    std::string_view text, std::string_view field_name, std::string* error) {
  return ExtractJsonComposite(text, field_name, '{', '}', error);
}

std::optional<std::string_view> ExtractJsonArray(
    std::string_view text, std::string_view field_name, std::string* error) {
  return ExtractJsonComposite(text, field_name, '[', ']', error);
}

int CountTopLevelObjects(std::string_view array_text) {
  int count = 0;
  int depth = 0;
  bool in_string = false;
  bool escaped = false;
  for (char current : array_text) {
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
        ++count;
      }
      ++depth;
    } else if (current == '}') {
      --depth;
    }
  }
  return count;
}

std::filesystem::path ResolveSiblingFile(const std::filesystem::path& base_path,
                                         const std::string& relative_path) {
  const std::filesystem::path path(relative_path);
  if (path.is_absolute()) {
    return path;
  }
  return base_path / path;
}

bool ApplyOptionalString(std::string_view text, std::string_view field_name,
                         std::string* value, std::string* error) {
  const ParseStringResult parsed = ExtractOptionalJsonStringField(text,
                                                                  field_name);
  if (!parsed.ok) {
    *error = parsed.error;
    return false;
  }
  if (parsed.found) {
    *value = parsed.value;
  }
  return true;
}

bool ApplyOptionalInt(std::string_view text, std::string_view field_name,
                      int* value, std::string* error) {
  const ParseIntResult parsed = ExtractOptionalJsonIntField(text, field_name);
  if (!parsed.ok) {
    *error = parsed.error;
    return false;
  }
  if (parsed.found) {
    *value = parsed.value;
  }
  return true;
}

bool ApplyOptionalBool(std::string_view text, std::string_view field_name,
                       bool* value, std::string* error) {
  const ParseBoolResult parsed = ExtractOptionalJsonBoolField(text,
                                                              field_name);
  if (!parsed.ok) {
    *error = parsed.error;
    return false;
  }
  if (parsed.found) {
    *value = parsed.value;
  }
  return true;
}

VisualMapLoadResult LoadVisualLayers(const std::filesystem::path& path,
                                     VisualMapData* data) {
  const ReadFileResult file = ReadTextFile(path);
  if (!file.ok) {
    return {false, true, {}, file.error};
  }

  std::string error;
  if (!ApplyOptionalInt(file.text, "width", &data->size.width, &error) ||
      !ApplyOptionalInt(file.text, "height", &data->size.height, &error) ||
      !ApplyOptionalInt(file.text, "tile_size_px", &data->size.tile_size,
                        &error) ||
      !ApplyOptionalInt(file.text, "unique_tile_ids",
                        &data->unique_tile_id_count, &error)) {
    return {false, true, {}, error};
  }

  const std::optional<std::string_view> layers =
      ExtractJsonArray(file.text, "layers", &error);
  if (!error.empty()) {
    return {false, true, {}, error};
  }
  if (layers.has_value()) {
    data->visual_layer_count = CountTopLevelObjects(*layers);
  }
  return {true, true, {}, {}};
}

VisualMapLoadResult LoadVisualObjects(const std::filesystem::path& path,
                                      VisualMapData* data) {
  const ReadFileResult file = ReadTextFile(path);
  if (!file.ok) {
    return {false, true, {}, file.error};
  }

  std::string error;
  if (!ApplyOptionalInt(file.text, "total", &data->visual_object_count,
                        &error)) {
    return {false, true, {}, error};
  }
  if (data->visual_object_count <= 0) {
    const std::optional<std::string_view> items =
        ExtractJsonArray(file.text, "items", &error);
    if (!error.empty()) {
      return {false, true, {}, error};
    }
    if (items.has_value()) {
      data->visual_object_count = CountTopLevelObjects(*items);
    }
  }
  return {true, true, {}, {}};
}

VisualMapLoadResult LoadVisualChunks(const std::filesystem::path& path,
                                     VisualMapData* data) {
  const ReadFileResult file = ReadTextFile(path);
  if (!file.ok) {
    return {false, true, {}, file.error};
  }

  std::string error;
  if (!ApplyOptionalInt(file.text, "chunk_size_tiles",
                        &data->chunk_size_tiles, &error) ||
      !ApplyOptionalInt(file.text, "total", &data->visual_chunk_count,
                        &error)) {
    return {false, true, {}, error};
  }
  if (data->visual_chunk_count <= 0) {
    const std::optional<std::string_view> items =
        ExtractJsonArray(file.text, "items", &error);
    if (!error.empty()) {
      return {false, true, {}, error};
    }
    if (items.has_value()) {
      data->visual_chunk_count = CountTopLevelObjects(*items);
    }
  }
  return {true, true, {}, {}};
}

}  // namespace

VisualMapLoadResult VisualMapLoader::Load(
    const std::filesystem::path& manifest_path,
    const LevelSize& raw_size) const {
  std::error_code file_error;
  if (!std::filesystem::exists(manifest_path, file_error)) {
    if (file_error) {
      return {false, false, {},
              "failed to inspect visual_map manifest: " +
                  file_error.message()};
    }
    return {true, false, {}, {}};
  }

  const ReadFileResult manifest = ReadTextFile(manifest_path);
  if (!manifest.ok) {
    return {false, true, {}, manifest.error};
  }

  VisualMapData data;
  data.loaded = true;
  data.manifest_path = manifest_path;
  data.base_path = manifest_path.parent_path();

  std::string error;
  if (!ApplyOptionalString(manifest.text, "schema_version",
                           &data.schema_version, &error) ||
      !ApplyOptionalString(manifest.text, "visual_generator_version",
                           &data.generator_version, &error) ||
      !ApplyOptionalInt(manifest.text, "width_tiles", &data.size.width,
                        &error) ||
      !ApplyOptionalInt(manifest.text, "height_tiles", &data.size.height,
                        &error) ||
      !ApplyOptionalInt(manifest.text, "tile_size_px", &data.size.tile_size,
                        &error) ||
      !ApplyOptionalBool(manifest.text, "changes_gameplay",
                         &data.changes_gameplay, &error) ||
      !ApplyOptionalBool(manifest.text, "moves_markers", &data.moves_markers,
                         &error) ||
      !ApplyOptionalBool(manifest.text, "changes_collision",
                         &data.changes_collision, &error)) {
    return {false, true, {}, error};
  }

  const std::optional<std::string_view> visual_profile =
      ExtractJsonObject(manifest.text, "visual_profile", &error);
  if (!error.empty()) {
    return {false, true, {}, error};
  }
  if (visual_profile.has_value()) {
    if (!ApplyOptionalString(*visual_profile, "id", &data.visual_profile_id,
                             &error) ||
        !ApplyOptionalString(*visual_profile, "name",
                             &data.visual_profile_name, &error)) {
      return {false, true, {}, error};
    }
  }

  std::string visual_layers_path = "visual_layers.json";
  std::string visual_objects_path = "visual_objects.json";
  std::string visual_chunks_path = "visual_chunks.json";
  const std::optional<std::string_view> files =
      ExtractJsonObject(manifest.text, "files", &error);
  if (!error.empty()) {
    return {false, true, {}, error};
  }
  if (files.has_value()) {
    if (!ApplyOptionalString(*files, "visual_layers", &visual_layers_path,
                             &error) ||
        !ApplyOptionalString(*files, "visual_objects", &visual_objects_path,
                             &error) ||
        !ApplyOptionalString(*files, "visual_chunks", &visual_chunks_path,
                             &error)) {
      return {false, true, {}, error};
    }
  }

  const VisualMapLoadResult layers = LoadVisualLayers(
      ResolveSiblingFile(data.base_path, visual_layers_path), &data);
  if (!layers.ok) {
    return layers;
  }
  const VisualMapLoadResult objects = LoadVisualObjects(
      ResolveSiblingFile(data.base_path, visual_objects_path), &data);
  if (!objects.ok) {
    return objects;
  }
  const VisualMapLoadResult chunks = LoadVisualChunks(
      ResolveSiblingFile(data.base_path, visual_chunks_path), &data);
  if (!chunks.ok) {
    return chunks;
  }

  if (!data.ValidateAgainstRawSize(raw_size, &error)) {
    return {false, true, {}, error};
  }

  if (data.visual_layer_count <= 0) {
    data.warnings.push_back("visual_map has no visual layers");
  }
  if (data.visual_object_count <= 0) {
    data.warnings.push_back("visual_map has no visual objects");
  }
  if (data.visual_chunk_count <= 0) {
    data.warnings.push_back("visual_map has no visual chunks");
  }

  return {true, true, data, {}};
}

}  // namespace sar::visual_pipeline
