/**
 * @file src/visual_pipeline/object_visual_plan.cpp
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains
 * implementation for object_visual_plan.cpp.
 */

#include "visual_pipeline/object_visual_plan.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace sar::visual_pipeline {
namespace {

/**
 * @brief Checks whether the container includes contains.
 */
bool Contains(std::string_view text, std::string_view needle) {
  return text.find(needle) != std::string_view::npos;
}

/**
 * @brief Starts s with.
 */
bool StartsWith(std::string_view text, std::string_view prefix) {
  return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

/**
 * @brief Checks whether tag is present.
 */
bool HasTag(const RuntimeObject& object, std::string_view tag) {
  return std::find(object.tags.begin(), object.tags.end(), tag) !=
         object.tags.end();
}

/**
 * @brief Checks whether any tag is present.
 */
bool HasAnyTag(const RuntimeObject& object,
               const std::vector<std::string_view>& tags) {
  for (const std::string_view tag : tags) {
    if (HasTag(object, tag)) {
      return true;
    }
  }
  return false;
}

/**
 * @brief Returns variant suffix.
 */
std::string VariantSuffix(const RuntimeObject& object, int variant_count) {
  if (variant_count <= 0) {
    return "01";
  }

  std::uint32_t hash = 2166136261U;
  for (const char ch : object.id) {
    hash ^= static_cast<unsigned char>(ch);
    hash *= 16777619U;
  }
  const int variant = static_cast<int>(hash %
                                       static_cast<std::uint32_t>(variant_count)) +
                      1;
  if (variant < 10) {
    return "0" + std::to_string(variant);
  }
  return std::to_string(variant);
}

/**
 * @brief Stores mapping result data shared between runtime systems.
 */
struct MappingResult {
  std::string sprite_family;
  ObjectVisualKind kind = ObjectVisualKind::kTypedFallback;
  int variant_count = 1;
  bool typed_fallback = false;
};

/**
 * @brief Returns exact type mapping.
 */
MappingResult ExactTypeMapping(const RuntimeObject& object) {
  const std::string& type = object.type;

  if (type == "bush_thicket") {
    return {"object.bush_thicket", ObjectVisualKind::kVegetation, 3, false};
  }
  if (type == "fallen_log") {
    return {"object.fallen_log", ObjectVisualKind::kWood, 3, false};
  }
  if (type == "big_dead_tree") {
    return {"object.big_dead_tree", ObjectVisualKind::kWood, 2, false};
  }
  if (type == "stone_chunk") {
    return {"object.stone_chunk", ObjectVisualKind::kStone, 4, false};
  }
  if (type == "scrap_pile") {
    return {"object.scrap_pile", ObjectVisualKind::kScrap, 3, false};
  }
  if (type == "rusted_barrel") {
    return {"object.rusted_barrel", ObjectVisualKind::kScrap, 2, false};
  }
  if (type == "cable_spool") {
    return {"object.cable_spool", ObjectVisualKind::kScrap, 2, false};
  }
  if (type == "broken_generator") {
    return {"object.broken_generator", ObjectVisualKind::kScrap, 2, false};
  }
  if (type == "broken_radio_mast") {
    return {"object.broken_radio_mast", ObjectVisualKind::kLandmark, 1,
            false};
  }
  if (type == "dead_campfire") {
    return {"object.dead_campfire", ObjectVisualKind::kCamp, 2, false};
  }
  if (type == "field_tent") {
    return {"object.field_tent", ObjectVisualKind::kCamp, 2, false};
  }
  if (type == "abandoned_backpack") {
    return {"object.abandoned_backpack", ObjectVisualKind::kCamp, 2, false};
  }
  if (type == "ammo_cache" || type == "medkit_cache") {
    return {"object." + type, ObjectVisualKind::kCache, 2, false};
  }
  if (type == "warning_sign") {
    return {"object.warning_sign", ObjectVisualKind::kLandmark, 2, false};
  }
  if (type == "car_wreck") {
    return {"object.car_wreck", ObjectVisualKind::kStructure, 2, false};
  }
  if (type == "buried_bunker_2x2" || type == "buried_bunker_2x3") {
    return {"structure." + type, ObjectVisualKind::kStructure, 1, false};
  }
  if (type == "watchtower") {
    return {"structure.watchtower", ObjectVisualKind::kStructure, 1, false};
  }
  if (type == "old_checkpoint") {
    return {"structure.old_checkpoint", ObjectVisualKind::kStructure, 1,
            false};
  }
  if (type == "wooden_bridge") {
    return {"structure.wooden_bridge", ObjectVisualKind::kStructure, 1,
            false};
  }
  if (type == "ruin_platform") {
    return {"structure.ruin_platform", ObjectVisualKind::kRuin, 2, false};
  }
  if (type == "old_well") {
    return {"structure.old_well", ObjectVisualKind::kLandmark, 1, false};
  }
  if (type == "old_grave_marker") {
    return {"object.old_grave_marker", ObjectVisualKind::kStone, 2, false};
  }
  if (type == "ancient_beacon") {
    return {"structure.ancient_beacon", ObjectVisualKind::kLandmark, 1,
            false};
  }
  if (type == "stone_ramp" || type == "stone_stairs") {
    return {"object." + type, ObjectVisualKind::kElevation, 2, false};
  }
  if (type == "trench" || type == "earth_berm" || type == "pit" ||
      type == "hill") {
    return {"object." + type, ObjectVisualKind::kElevation, 3, false};
  }
  if (type == "abandoned_cart") {
    return {"object.abandoned_cart", ObjectVisualKind::kWood, 2, false};
  }

  return {};
}

/**
 * @brief Returns fallback mapping.
 */
MappingResult FallbackMapping(const RuntimeObject& object) {
  const std::string key = object.type + " " + object.family;

  if (Contains(key, "tree") || Contains(key, "bush") ||
      Contains(key, "vegetation") || Contains(key, "forest")) {
    return {"object.fallback_vegetation", ObjectVisualKind::kVegetation, 3,
            true};
  }
  if (Contains(key, "stone") || Contains(key, "grave") ||
      Contains(key, "rock")) {
    return {"object.fallback_stone", ObjectVisualKind::kStone, 3, true};
  }
  if (Contains(key, "log") || Contains(key, "wood") ||
      Contains(key, "cart") || Contains(key, "bridge")) {
    return {"object.fallback_wood", ObjectVisualKind::kWood, 3, true};
  }
  if (Contains(key, "scrap") || Contains(key, "barrel") ||
      Contains(key, "generator") || Contains(key, "radio") ||
      Contains(key, "cable")) {
    return {"object.fallback_scrap", ObjectVisualKind::kScrap, 3, true};
  }
  if (Contains(key, "camp") || Contains(key, "tent") ||
      Contains(key, "fire") || HasTag(object, "camp")) {
    return {"object.fallback_camp", ObjectVisualKind::kCamp, 2, true};
  }
  if (Contains(key, "cache") || HasAnyTag(object, {"loot", "cache"})) {
    return {"object.fallback_cache", ObjectVisualKind::kCache, 2, true};
  }
  if (Contains(key, "ruin") || Contains(key, "wall") ||
      Contains(key, "bunker") || Contains(key, "checkpoint")) {
    return {"structure.fallback_ruin", ObjectVisualKind::kRuin, 3, true};
  }
  if (Contains(key, "elevation") || Contains(key, "ramp") ||
      Contains(key, "stairs") || Contains(key, "trench") ||
      Contains(key, "berm") || Contains(key, "hill") ||
      Contains(key, "pit")) {
    return {"object.fallback_elevation", ObjectVisualKind::kElevation, 3,
            true};
  }
  if (Contains(key, "landmark") || HasTag(object, "landmark")) {
    return {"object.fallback_landmark", ObjectVisualKind::kLandmark, 2,
            true};
  }
  if (object.blocks_movement || object.blocks_vision ||
      object.blocks_projectiles || HasTag(object, "cover")) {
    return {"object.fallback_cover", ObjectVisualKind::kCover, 3, true};
  }

  return {"object.fallback_debris", ObjectVisualKind::kTypedFallback, 3,
          true};
}

/**
 * @brief Executes the map object operation.
 */
MappingResult MapObject(const RuntimeObject& object) {
  MappingResult result = ExactTypeMapping(object);
  if (!result.sprite_family.empty()) {
    return result;
  }
  return FallbackMapping(object);
}

/**
 * @brief Draws layer for object.
 */
std::string DrawLayerForObject(const RuntimeObject& object,
                               ObjectVisualKind kind) {
  if (StartsWith(object.type, "big_dead_tree") ||
      StartsWith(object.type, "watchtower") ||
      StartsWith(object.type, "buried_bunker") ||
      object.height > 2) {
    return "tall_object";
  }
  if (kind == ObjectVisualKind::kStructure || kind == ObjectVisualKind::kRuin) {
    return "structure";
  }
  if (kind == ObjectVisualKind::kElevation) {
    return "terrain_overlay";
  }
  return "object";
}

/**
 * @brief Checks whether object inside is true.
 */
bool IsObjectInside(const RuntimeObject& object, const LevelSize& size) {
  return object.x >= 0 && object.y >= 0 && object.width > 0 &&
         object.height > 0 && object.x + object.width <= size.width &&
         object.y + object.height <= size.height;
}

}  // namespace

/**
 * @brief Returns object visual kind name.
 */
const char* ObjectVisualKindName(ObjectVisualKind kind) {
  switch (kind) {
    case ObjectVisualKind::kUnknown:
      return "unknown";
    case ObjectVisualKind::kVegetation:
      return "vegetation";
    case ObjectVisualKind::kWood:
      return "wood";
    case ObjectVisualKind::kStone:
      return "stone";
    case ObjectVisualKind::kScrap:
      return "scrap";
    case ObjectVisualKind::kCamp:
      return "camp";
    case ObjectVisualKind::kCache:
      return "cache";
    case ObjectVisualKind::kStructure:
      return "structure";
    case ObjectVisualKind::kRuin:
      return "ruin";
    case ObjectVisualKind::kElevation:
      return "elevation";
    case ObjectVisualKind::kLandmark:
      return "landmark";
    case ObjectVisualKind::kCover:
      return "cover";
    case ObjectVisualKind::kTypedFallback:
      return "typed_fallback";
  }
  return "unknown";
}

/**
 * @brief Builds a readable diagnostic dump for dump.
 */
std::string ObjectVisualSummary::Dump() const {
  std::ostringstream stream;
  stream << "ObjectVisualSummary { source_object_count: "
         << source_object_count
         << ", mapped_object_count: " << mapped_object_count
         << ", typed_fallback_count: " << typed_fallback_count
         << ", generic_object_count: " << generic_object_count
         << ", missing_sprite_uses: " << missing_sprite_uses
         << ", skipped_sprites: " << skipped_sprites << " }";
  return stream.str();
}

/**
 * @brief Checks whether valid is true.
 */
bool ObjectVisualPlan::IsValid() const {
  const int expected_size = size.width * size.height;
  return size.width > 0 && size.height > 0 && size.tile_size > 0 &&
         expected_size > 0;
}

/**
 * @brief Builds a readable diagnostic dump for dump.
 */
std::string ObjectVisualPlan::Dump() const {
  std::ostringstream stream;
  stream << "ObjectVisualPlan { size: " << size.width << 'x' << size.height
         << ", items: " << items.size()
         << ", summary: " << summary.Dump() << " }";
  return stream.str();
}

/**
 * @brief Builds object visual plan.
 */
ObjectVisualPlan BuildObjectVisualPlan(const LevelData& level,
                                       std::string* error) {
  ObjectVisualPlan plan;
  plan.size = level.size;

  if (level.size.width <= 0 || level.size.height <= 0 ||
      level.size.tile_size <= 0) {
    if (error != nullptr) {
      *error = "invalid level size for object visual plan";
    }
    return {};
  }

  plan.items.reserve(level.objects.size());
  plan.summary.source_object_count = static_cast<int>(level.objects.size());

  for (const RuntimeObject& object : level.objects) {
    ++plan.summary.source_type_counts[object.type];
    if (!IsObjectInside(object, level.size)) {
      ++plan.summary.skipped_sprites;
      continue;
    }

    const MappingResult mapping = MapObject(object);
    if (mapping.sprite_family.empty()) {
      ++plan.summary.missing_sprite_uses;
      continue;
    }

    ObjectVisualItem item;
    item.id = object.id;
    item.source_type = object.type;
    item.source_family = object.family;
    item.sprite_family = mapping.sprite_family;
    item.sprite_id = mapping.sprite_family + "." +
                     VariantSuffix(object, mapping.variant_count);
    item.kind = mapping.kind;
    item.x = object.x;
    item.y = object.y;
    item.width = object.width;
    item.height = object.height;
    item.sort_y = object.y + object.height;
    item.draw_layer = DrawLayerForObject(object, mapping.kind);

    if (mapping.typed_fallback) {
      ++plan.summary.typed_fallback_count;
    }
    ++plan.summary.sprite_family_counts[item.sprite_family];
    ++plan.summary.visual_kind_counts[ObjectVisualKindName(item.kind)];
    plan.items.push_back(std::move(item));
  }

  plan.summary.mapped_object_count = static_cast<int>(plan.items.size());
  plan.summary.generic_object_count = 0;
  return plan;
}

}  // namespace sar::visual_pipeline
