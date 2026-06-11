/**
 * @file src/render3d/model_registry.cpp
 * @brief 3D renderer, camera, player movement, fog, and asset registry. Contains implementation
 * for model_registry.cpp.
 */

#include "render3d/model_registry.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

namespace sar::render3d {
namespace {

/**
 * @brief Defines the supported JSON type 3D values.
 */
enum class JsonType3D {
  kNull,
  kBool,
  kNumber,
  kString,
  kObject,
  kArray,
};

/**
 * @brief Stores JSON value 3D data shared between runtime systems.
 */
struct JsonValue3D {
  JsonType3D type = JsonType3D::kNull;
  bool bool_value = false;
  double number_value = 0.0;
  std::string string_value;
  std::map<std::string, JsonValue3D> object_value;
  std::vector<JsonValue3D> array_value;
};

/**
 * @brief Stores JSON parse result 3D data shared between runtime systems.
 */
struct JsonParseResult3D {
  bool ok = false;
  JsonValue3D value;
  std::string error;
};

/**
 * @brief Owns the JSON parser 3D behavior and its runtime state.
 */
class JsonParser3D {
 public:
  explicit JsonParser3D(std::string_view text) : text_(text) {}

  JsonParseResult3D Parse() {
    JsonValue3D value;
    if (!ParseValue(&value)) {
      return {false, {}, error_};
    }
    SkipWhitespace();
    if (position_ != text_.size()) {
      return {false, {}, "unexpected trailing JSON content"};
    }
    return {true, std::move(value), {}};
  }

 private:
  void SkipWhitespace() {
    while (position_ < text_.size() &&
           std::isspace(static_cast<unsigned char>(text_[position_])) != 0) {
      ++position_;
    }
  }

  bool Consume(char expected) {
    SkipWhitespace();
    if (position_ >= text_.size() || text_[position_] != expected) {
      return false;
    }
    ++position_;
    return true;
  }

  bool MatchLiteral(std::string_view literal) {
    SkipWhitespace();
    if (text_.substr(position_, literal.size()) != literal) {
      return false;
    }
    position_ += literal.size();
    return true;
  }

  bool ParseValue(JsonValue3D* value) {
    SkipWhitespace();
    if (position_ >= text_.size()) {
      error_ = "unexpected end of JSON";
      return false;
    }

    const char current = text_[position_];
    if (current == '{') {
      return ParseObject(value);
    }
    if (current == '[') {
      return ParseArray(value);
    }
    if (current == '"') {
      std::string result;
      if (!ParseString(&result)) {
        return false;
      }
      value->type = JsonType3D::kString;
      value->string_value = std::move(result);
      return true;
    }
    if (current == '-' || std::isdigit(static_cast<unsigned char>(current)) != 0) {
      return ParseNumber(value);
    }
    if (MatchLiteral("true")) {
      value->type = JsonType3D::kBool;
      value->bool_value = true;
      return true;
    }
    if (MatchLiteral("false")) {
      value->type = JsonType3D::kBool;
      value->bool_value = false;
      return true;
    }
    if (MatchLiteral("null")) {
      value->type = JsonType3D::kNull;
      return true;
    }

    error_ = "unsupported JSON value";
    return false;
  }

  bool ParseObject(JsonValue3D* value) {
    if (!Consume('{')) {
      error_ = "expected JSON object";
      return false;
    }

    value->type = JsonType3D::kObject;
    SkipWhitespace();
    if (Consume('}')) {
      return true;
    }

    while (position_ < text_.size()) {
      std::string key;
      if (!ParseString(&key)) {
        return false;
      }
      if (!Consume(':')) {
        error_ = "expected colon after object key";
        return false;
      }
      JsonValue3D field_value;
      if (!ParseValue(&field_value)) {
        return false;
      }
      value->object_value[std::move(key)] = std::move(field_value);

      SkipWhitespace();
      if (Consume('}')) {
        return true;
      }
      if (!Consume(',')) {
        error_ = "expected comma between object fields";
        return false;
      }
    }

    error_ = "unterminated JSON object";
    return false;
  }

  bool ParseArray(JsonValue3D* value) {
    if (!Consume('[')) {
      error_ = "expected JSON array";
      return false;
    }

    value->type = JsonType3D::kArray;
    SkipWhitespace();
    if (Consume(']')) {
      return true;
    }

    while (position_ < text_.size()) {
      JsonValue3D element;
      if (!ParseValue(&element)) {
        return false;
      }
      value->array_value.push_back(std::move(element));

      SkipWhitespace();
      if (Consume(']')) {
        return true;
      }
      if (!Consume(',')) {
        error_ = "expected comma between array elements";
        return false;
      }
    }

    error_ = "unterminated JSON array";
    return false;
  }

  bool ParseString(std::string* value) {
    SkipWhitespace();
    if (position_ >= text_.size() || text_[position_] != '"') {
      error_ = "expected JSON string";
      return false;
    }
    ++position_;

    std::string result;
    while (position_ < text_.size()) {
      const char current = text_[position_++];
      if (current == '"') {
        *value = std::move(result);
        return true;
      }
      if (current != '\\') {
        result.push_back(current);
        continue;
      }
      if (position_ >= text_.size()) {
        error_ = "unterminated JSON escape sequence";
        return false;
      }
      const char escaped = text_[position_++];
      switch (escaped) {
        case '"':
        case '\\':
        case '/':
          result.push_back(escaped);
          break;
        case 'b':
          result.push_back('\b');
          break;
        case 'f':
          result.push_back('\f');
          break;
        case 'n':
          result.push_back('\n');
          break;
        case 'r':
          result.push_back('\r');
          break;
        case 't':
          result.push_back('\t');
          break;
        default:
          error_ = "unsupported JSON escape sequence";
          return false;
      }
    }

    error_ = "unterminated JSON string";
    return false;
  }

  bool ParseNumber(JsonValue3D* value) {
    SkipWhitespace();
    const std::size_t start = position_;
    if (position_ < text_.size() && text_[position_] == '-') {
      ++position_;
    }
    while (position_ < text_.size() &&
           std::isdigit(static_cast<unsigned char>(text_[position_])) != 0) {
      ++position_;
    }
    if (position_ < text_.size() && text_[position_] == '.') {
      ++position_;
      while (position_ < text_.size() &&
             std::isdigit(static_cast<unsigned char>(text_[position_])) != 0) {
        ++position_;
      }
    }
    if (position_ < text_.size() &&
        (text_[position_] == 'e' || text_[position_] == 'E')) {
      ++position_;
      if (position_ < text_.size() &&
          (text_[position_] == '+' || text_[position_] == '-')) {
        ++position_;
      }
      while (position_ < text_.size() &&
             std::isdigit(static_cast<unsigned char>(text_[position_])) != 0) {
        ++position_;
      }
    }

    double number = 0.0;
    const auto result = std::from_chars(text_.data() + start,
                                        text_.data() + position_, number);
    if (result.ec != std::errc()) {
      error_ = "invalid JSON number";
      return false;
    }

    value->type = JsonType3D::kNumber;
    value->number_value = number;
    return true;
  }

  std::string_view text_;
  std::size_t position_ = 0;
  std::string error_;
};

/**
 * @brief Reads text file.
 */
std::string ReadTextFile(const std::filesystem::path& path,
                         std::string* error) {
  std::ifstream input(path);
  if (!input.is_open()) {
    *error = "failed to open file: " + path.string();
    return {};
  }

  std::ostringstream stream;
  stream << input.rdbuf();
  return stream.str();
}

/**
 * @brief Executes the object field operation.
 */
const JsonValue3D* ObjectField(const JsonValue3D& object,
                               std::string_view name) {
  if (object.type != JsonType3D::kObject) {
    return nullptr;
  }
  const auto found = object.object_value.find(std::string(name));
  if (found == object.object_value.end()) {
    return nullptr;
  }
  return &found->second;
}

/**
 * @brief Executes the string field operation.
 */
std::string StringField(const JsonValue3D& object, std::string_view name,
                        std::string fallback) {
  const JsonValue3D* value = ObjectField(object, name);
  if (value == nullptr || value->type != JsonType3D::kString) {
    return fallback;
  }
  return value->string_value;
}

/**
 * @brief Executes the float field operation.
 */
float FloatField(const JsonValue3D& object, std::string_view name,
                 float fallback) {
  const JsonValue3D* value = ObjectField(object, name);
  if (value == nullptr || value->type != JsonType3D::kNumber) {
    return fallback;
  }
  return static_cast<float>(value->number_value);
}

/**
 * @brief Executes the int field operation.
 */
int IntField(const JsonValue3D& object, std::string_view name, int fallback) {
  const JsonValue3D* value = ObjectField(object, name);
  if (value == nullptr || value->type != JsonType3D::kNumber) {
    return fallback;
  }
  return static_cast<int>(value->number_value);
}

/**
 * @brief Executes the bool field operation.
 */
bool BoolField(const JsonValue3D& object, std::string_view name,
               bool fallback) {
  const JsonValue3D* value = ObjectField(object, name);
  if (value == nullptr || value->type != JsonType3D::kBool) {
    return fallback;
  }
  return value->bool_value;
}

/**
 * @brief Executes the string array field operation.
 */
std::vector<std::string> StringArrayField(const JsonValue3D& object,
                                          std::string_view name) {
  std::vector<std::string> result;
  const JsonValue3D* value = ObjectField(object, name);
  if (value == nullptr || value->type != JsonType3D::kArray) {
    return result;
  }
  for (const JsonValue3D& element : value->array_value) {
    if (element.type == JsonType3D::kString) {
      result.push_back(element.string_value);
    }
  }
  return result;
}

/**
 * @brief Executes the float range field operation.
 */
std::pair<float, float> FloatRangeField(const JsonValue3D& object,
                                        std::string_view name,
                                        float fallback_min,
                                        float fallback_max) {
  const JsonValue3D* value = ObjectField(object, name);
  if (value == nullptr || value->type != JsonType3D::kArray ||
      value->array_value.size() < 2 ||
      value->array_value[0].type != JsonType3D::kNumber ||
      value->array_value[1].type != JsonType3D::kNumber) {
    return {fallback_min, fallback_max};
  }

  float first = static_cast<float>(value->array_value[0].number_value);
  float second = static_cast<float>(value->array_value[1].number_value);
  if (second < first) {
    std::swap(first, second);
  }
  return {first, second};
}

/**
 * @brief Executes the int range field operation.
 */
std::pair<int, int> IntRangeField(const JsonValue3D& object,
                                  std::string_view name,
                                  int fallback_min,
                                  int fallback_max) {
  const JsonValue3D* value = ObjectField(object, name);
  if (value == nullptr || value->type != JsonType3D::kArray ||
      value->array_value.size() < 2 ||
      value->array_value[0].type != JsonType3D::kNumber ||
      value->array_value[1].type != JsonType3D::kNumber) {
    return {fallback_min, fallback_max};
  }

  int first = static_cast<int>(value->array_value[0].number_value);
  int second = static_cast<int>(value->array_value[1].number_value);
  if (second < first) {
    std::swap(first, second);
  }
  return {first, second};
}

/**
 * @brief Parses selector mode from external data.
 */
std::optional<ModelSelectorMode3D> ParseSelectorMode(std::string_view value) {
  if (value == "fixed") {
    return ModelSelectorMode3D::kFixed;
  }
  if (value == "random") {
    return ModelSelectorMode3D::kRandom;
  }
  if (value == "weighted_random" || value == "weighted") {
    return ModelSelectorMode3D::kWeightedRandom;
  }
  if (value == "named") {
    return ModelSelectorMode3D::kNamed;
  }
  if (value == "by_tag" || value == "tag") {
    return ModelSelectorMode3D::kByTag;
  }
  return std::nullopt;
}

/**
 * @brief Parses placement mode from external data.
 */
std::optional<ModelPlacementMode3D> ParsePlacementMode(std::string_view value) {
  if (value == "single") {
    return ModelPlacementMode3D::kSingle;
  }
  if (value == "cluster") {
    return ModelPlacementMode3D::kCluster;
  }
  return std::nullopt;
}

/**
 * @brief Checks whether all tags is present.
 */
bool HasAllTags(const ModelAsset3D& asset,
                const std::vector<std::string>& required_tags) {
  for (const std::string& required : required_tags) {
    if (std::find(asset.tags.begin(), asset.tags.end(), required) ==
        asset.tags.end()) {
      return false;
    }
  }
  return true;
}

/**
 * @brief Executes the mix seed operation.
 */
std::uint64_t MixSeed(std::uint64_t value) {
  value ^= value >> 33U;
  value *= 0xff51afd7ed558ccdULL;
  value ^= value >> 33U;
  value *= 0xc4ceb9fe1a85ec53ULL;
  value ^= value >> 33U;
  return value;
}

/**
 * @brief Checks whether hash string is present.
 */
std::uint64_t HashString(std::string_view text) {
  std::uint64_t hash = 1469598103934665603ULL;
  for (char character : text) {
    hash ^= static_cast<unsigned char>(character);
    hash *= 1099511628211ULL;
  }
  return hash;
}

/**
 * @brief Parses JSON file from external data.
 */
JsonParseResult3D ParseJsonFile(const std::filesystem::path& path) {
  std::string error;
  const std::string text = ReadTextFile(path, &error);
  if (!error.empty()) {
    return {false, {}, error};
  }
  return JsonParser3D(text).Parse();
}

/**
 * @brief Loads asset library.
 */
bool LoadAssetLibrary(const std::filesystem::path& path,
                      ModelRegistry3D* registry,
                      std::vector<std::string>* warnings,
                      std::string* error) {
  JsonParseResult3D parsed = ParseJsonFile(path);
  if (!parsed.ok) {
    *error = path.string() + ": " + parsed.error;
    return false;
  }

  const JsonValue3D* models = ObjectField(parsed.value, "models");
  if (models == nullptr || models->type != JsonType3D::kObject) {
    warnings->push_back("asset library has no models object: " + path.string());
    return true;
  }

  for (const auto& [model_id, model_json] : models->object_value) {
    if (model_json.type != JsonType3D::kObject) {
      warnings->push_back("model entry is not an object: " + model_id);
      continue;
    }

    ModelAsset3D asset;
    asset.id = model_id;
    asset.path = StringField(model_json, "path", "");
    asset.tags = StringArrayField(model_json, "tags");
    asset.default_scale = FloatField(model_json, "default_scale", 1.0F);
    asset.vertical_offset = FloatField(model_json, "vertical_offset", 0.0F);
    asset.fallback = StringField(model_json, "fallback", "cube_debug");
    if (asset.path.empty()) {
      warnings->push_back("model has empty path: " + model_id);
    }
    registry->assets[asset.id] = std::move(asset);
  }

  return true;
}

/**
 * @brief Parses variants from external data.
 */
std::vector<ModelVariant3D> ParseVariants(const JsonValue3D& binding_json) {
  std::vector<ModelVariant3D> variants;
  const JsonValue3D* variants_json = ObjectField(binding_json, "variants");
  if (variants_json == nullptr || variants_json->type != JsonType3D::kArray) {
    return variants;
  }

  for (const JsonValue3D& variant_json : variants_json->array_value) {
    ModelVariant3D variant;
    if (variant_json.type == JsonType3D::kString) {
      variant.model_id = variant_json.string_value;
    } else if (variant_json.type == JsonType3D::kObject) {
      variant.model_id = StringField(variant_json, "model", "");
      variant.weight = std::max(1, IntField(variant_json, "weight", 1));
    }
    if (!variant.model_id.empty()) {
      variants.push_back(std::move(variant));
    }
  }

  return variants;
}

/**
 * @brief Loads tileset.
 */
bool LoadTileset(const std::filesystem::path& path,
                 ModelRegistry3D* registry,
                 std::vector<std::string>* warnings,
                 std::string* error) {
  JsonParseResult3D parsed = ParseJsonFile(path);
  if (!parsed.ok) {
    *error = path.string() + ": " + parsed.error;
    return false;
  }

  registry->tileset.id = StringField(parsed.value, "tileset_id", "dark_forest_3d");
  const JsonValue3D* bindings = ObjectField(parsed.value, "bindings");
  if (bindings == nullptr || bindings->type != JsonType3D::kObject) {
    warnings->push_back("tileset has no bindings object: " + path.string());
    return true;
  }

  for (const auto& [semantic_key, binding_json] : bindings->object_value) {
    if (binding_json.type != JsonType3D::kObject) {
      warnings->push_back("tileset binding is not an object: " + semantic_key);
      continue;
    }

    ModelBinding3D binding;
    binding.semantic_key = semantic_key;
    binding.fixed_model_id = StringField(binding_json, "model", "");
    if (binding.fixed_model_id.empty()) {
      binding.fixed_model_id = StringField(binding_json, "fixed_model", "");
    }
    binding.variants = ParseVariants(binding_json);
    binding.required_tags = StringArrayField(binding_json, "tags");
    if (binding.required_tags.empty()) {
      binding.required_tags = StringArrayField(binding_json, "required_tags");
    }
    binding.random_rotation = BoolField(binding_json, "random_rotation", false);
    binding.vertical_offset = FloatField(binding_json, "vertical_offset", 0.0F);
    binding.spawn_chance = std::clamp(FloatField(binding_json, "spawn_chance", 1.0F),
                                      0.0F, 1.0F);
    binding.fallback = StringField(binding_json, "fallback", "cube_debug");

    const std::pair<int, int> count_range = IntRangeField(binding_json,
                                                          "count_range", 1, 1);
    binding.count_min = std::max(1, count_range.first);
    binding.count_max = std::max(binding.count_min, count_range.second);

    const std::pair<float, float> scale_range = FloatRangeField(
        binding_json, "scale_range", 1.0F, 1.0F);
    binding.scale_min = std::max(0.01F, scale_range.first);
    binding.scale_max = std::max(binding.scale_min, scale_range.second);

    const std::pair<float, float> offset_range = FloatRangeField(
        binding_json, "offset_range", 0.0F, 0.0F);
    binding.offset_min = offset_range.first;
    binding.offset_max = offset_range.second;

    const std::string selector = StringField(binding_json, "selector", "");
    if (!selector.empty()) {
      const std::optional<ModelSelectorMode3D> parsed_selector =
          ParseSelectorMode(selector);
      if (!parsed_selector.has_value()) {
        warnings->push_back("unsupported selector for binding " + semantic_key +
                            ": " + selector);
      } else {
        binding.selector = *parsed_selector;
      }
    } else if (!binding.fixed_model_id.empty()) {
      binding.selector = ModelSelectorMode3D::kFixed;
    }

    const std::string placement = StringField(binding_json, "placement", "");
    if (!placement.empty()) {
      const std::optional<ModelPlacementMode3D> parsed_placement =
          ParsePlacementMode(placement);
      if (!parsed_placement.has_value()) {
        warnings->push_back("unsupported placement for binding " + semantic_key +
                            ": " + placement);
      } else {
        binding.placement = *parsed_placement;
      }
    }

    registry->tileset.bindings[binding.semantic_key] = std::move(binding);
  }

  return true;
}

/**
 * @brief Validates registry references and reports failures.
 */
void ValidateRegistryReferences(ModelRegistry3D* registry,
                                std::vector<std::string>* warnings) {
  for (auto& [semantic_key, binding] : registry->tileset.bindings) {
    if (!binding.fixed_model_id.empty() &&
        registry->assets.find(binding.fixed_model_id) == registry->assets.end()) {
      warnings->push_back("binding " + semantic_key +
                          " references missing model: " + binding.fixed_model_id);
    }
    for (const ModelVariant3D& variant : binding.variants) {
      if (registry->assets.find(variant.model_id) == registry->assets.end()) {
        warnings->push_back("binding " + semantic_key +
                            " variant references missing model: " +
                            variant.model_id);
      }
    }
  }
}

}  // namespace

/**
 * @brief Returns model selector mode 3D name.
 */
const char* ModelSelectorMode3DName(ModelSelectorMode3D mode) {
  switch (mode) {
    case ModelSelectorMode3D::kFixed:
      return "fixed";
    case ModelSelectorMode3D::kRandom:
      return "random";
    case ModelSelectorMode3D::kWeightedRandom:
      return "weighted_random";
    case ModelSelectorMode3D::kNamed:
      return "named";
    case ModelSelectorMode3D::kByTag:
      return "by_tag";
  }
  return "weighted_random";
}

/**
 * @brief Returns model placement mode 3D name.
 */
const char* ModelPlacementMode3DName(ModelPlacementMode3D mode) {
  switch (mode) {
    case ModelPlacementMode3D::kSingle:
      return "single";
    case ModelPlacementMode3D::kCluster:
      return "cluster";
  }
  return "single";
}

/**
 * @brief Finds asset.
 */
const ModelAsset3D* ModelRegistry3D::FindAsset(
    std::string_view model_id) const {
  const auto found = assets.find(std::string(model_id));
  if (found == assets.end()) {
    return nullptr;
  }
  return &found->second;
}

/**
 * @brief Finds binding.
 */
const ModelBinding3D* ModelRegistry3D::FindBinding(
    std::string_view semantic_key) const {
  const auto found = tileset.bindings.find(std::string(semantic_key));
  if (found == tileset.bindings.end()) {
    return nullptr;
  }
  return &found->second;
}

/**
 * @brief Selects model ID.
 */
std::string ModelRegistry3D::SelectModelId(std::string_view semantic_key,
                                           std::uint64_t base_seed,
                                           int tile_x,
                                           int tile_y) const {
  const ModelBinding3D* binding = FindBinding(semantic_key);
  if (binding == nullptr) {
    return {};
  }

  if (binding->selector == ModelSelectorMode3D::kFixed ||
      binding->selector == ModelSelectorMode3D::kNamed) {
    return FindAsset(binding->fixed_model_id) != nullptr ? binding->fixed_model_id
                                                         : std::string{};
  }

  const std::uint64_t seed = DeterministicAssetSeed(base_seed, semantic_key,
                                                   tile_x, tile_y);
  if (binding->selector == ModelSelectorMode3D::kByTag) {
    std::vector<std::string> candidates;
    for (const auto& [model_id, asset] : assets) {
      if (HasAllTags(asset, binding->required_tags)) {
        candidates.push_back(model_id);
      }
    }
    if (candidates.empty()) {
      return {};
    }
    return candidates[static_cast<std::size_t>(seed % candidates.size())];
  }

  std::vector<ModelVariant3D> available_variants;
  for (const ModelVariant3D& variant : binding->variants) {
    if (FindAsset(variant.model_id) != nullptr) {
      available_variants.push_back(variant);
    }
  }
  if (available_variants.empty()) {
    return {};
  }

  if (binding->selector == ModelSelectorMode3D::kRandom) {
    return available_variants[static_cast<std::size_t>(
        seed % available_variants.size())].model_id;
  }

  int total_weight = 0;
  for (const ModelVariant3D& variant : available_variants) {
    total_weight += std::max(1, variant.weight);
  }
  int cursor = static_cast<int>(seed % static_cast<std::uint64_t>(total_weight));
  for (const ModelVariant3D& variant : available_variants) {
    cursor -= std::max(1, variant.weight);
    if (cursor < 0) {
      return variant.model_id;
    }
  }

  return available_variants.back().model_id;
}

/**
 * @brief Implements ModelRegistry3D::Summary.
 */
ModelRegistry3DSummary ModelRegistry3D::Summary() const {
  ModelRegistry3DSummary summary;
  summary.model_count = static_cast<int>(assets.size());
  summary.binding_count = static_cast<int>(tileset.bindings.size());
  for (const auto& [semantic_key, binding] : tileset.bindings) {
    (void)semantic_key;
    summary.variant_reference_count += static_cast<int>(binding.variants.size());
    if (!binding.fixed_model_id.empty() && FindAsset(binding.fixed_model_id) == nullptr) {
      ++summary.missing_reference_count;
    }
    for (const ModelVariant3D& variant : binding.variants) {
      if (FindAsset(variant.model_id) == nullptr) {
        ++summary.missing_reference_count;
      }
    }
  }
  return summary;
}

/**
 * @brief Builds a readable diagnostic dump for dump.
 */
std::string ModelRegistry3D::Dump() const {
  const ModelRegistry3DSummary summary = Summary();
  std::ostringstream stream;
  stream << "ModelRegistry3D { tileset: " << tileset.id
         << ", models: " << summary.model_count
         << ", bindings: " << summary.binding_count
         << ", variants: " << summary.variant_reference_count
         << ", missing_refs: " << summary.missing_reference_count << " }";
  return stream.str();
}

/**
 * @brief Loads model registry 3D.
 */
LoadModelRegistry3DResult LoadModelRegistry3D(
    const std::filesystem::path& asset_library_path,
    const std::filesystem::path& tileset_path) {
  LoadModelRegistry3DResult result;
  result.found = std::filesystem::exists(asset_library_path) ||
                 std::filesystem::exists(tileset_path);

  if (!std::filesystem::exists(asset_library_path)) {
    result.error = "3D asset library config not found: " +
                   asset_library_path.string();
    return result;
  }
  if (!std::filesystem::exists(tileset_path)) {
    result.error = "3D tileset config not found: " + tileset_path.string();
    return result;
  }

  std::string error;
  if (!LoadAssetLibrary(asset_library_path, &result.registry,
                        &result.warnings, &error)) {
    result.error = error;
    return result;
  }
  if (!LoadTileset(tileset_path, &result.registry, &result.warnings, &error)) {
    result.error = error;
    return result;
  }

  ValidateRegistryReferences(&result.registry, &result.warnings);
  result.ok = true;
  return result;
}

/**
 * @brief Executes the deterministic asset seed operation.
 */
std::uint64_t DeterministicAssetSeed(std::uint64_t base_seed,
                                     std::string_view semantic_key,
                                     int tile_x,
                                     int tile_y) {
  std::uint64_t seed = base_seed ^ HashString(semantic_key);
  seed ^= MixSeed(static_cast<std::uint64_t>(static_cast<std::uint32_t>(tile_x))
                  << 32U);
  seed ^= MixSeed(static_cast<std::uint64_t>(static_cast<std::uint32_t>(tile_y)));
  return MixSeed(seed);
}

}  // namespace sar::render3d
