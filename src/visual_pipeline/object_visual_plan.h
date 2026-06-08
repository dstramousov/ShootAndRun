#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_OBJECT_VISUAL_PLAN_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_OBJECT_VISUAL_PLAN_H_

/**
 * @file src/visual_pipeline/object_visual_plan.h
 * @brief Visual preparation pipeline data contracts, passes, and artifacts. Contains public
 * declarations for object_visual_plan.h.
 */

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "level/level_data.h"

namespace sar::visual_pipeline {

/**
 * @brief Resolved visual category for a runtime object.
 */
enum class ObjectVisualKind : std::uint8_t {
  kUnknown = 0,
  kVegetation = 1,
  kWood = 2,
  kStone = 3,
  kScrap = 4,
  kCamp = 5,
  kCache = 6,
  kStructure = 7,
  kRuin = 8,
  kElevation = 9,
  kLandmark = 10,
  kCover = 11,
  kTypedFallback = 12,
};

/**
 * @brief Returns the stable name for an object visual kind.
 *
 * @param kind Object visual kind value.
 * @return Stable lowercase visual kind name.
 */
const char* ObjectVisualKindName(ObjectVisualKind kind);

/**
 * @brief One resolved visual object placement.
 */
struct ObjectVisualItem {
  std::string id;  ///< Stable identifier loaded from source data or configuration.
  std::string source_type;  ///< Semantic type for source.
  std::string source_family;  ///< Source family value carried by this data structure.
  std::string sprite_family;  ///< Sprite family value carried by this data structure.
  std::string sprite_id;  ///< Stable identifier for sprite ID.
  std::string draw_layer;  ///< Draw layer value carried by this data structure.
  ObjectVisualKind kind = ObjectVisualKind::kUnknown;  ///< Kind value carried by this data structure.
  int x = 0;  ///< Tile, screen, or world coordinate for x.
  int y = 0;  ///< Tile, screen, or world coordinate for y.
  int width = 1;  ///< Size component for width.
  int height = 1;  ///< Signed elevation level for this tile or object.
  int sort_y = 0;  ///< Tile, screen, or world coordinate for sort y.
};

/**
 * @brief Counters produced by the runtime object visual planning pass.
 */
struct ObjectVisualSummary {
  int source_object_count = 0;  ///< Count of source object count entries or events.
  int mapped_object_count = 0;  ///< Count of mapped object count entries or events.
  int typed_fallback_count = 0;  ///< Count of typed fallback count entries or events.
  int generic_object_count = 0;  ///< Count of generic object count entries or events.
  int missing_sprite_uses = 0;  ///< Missing sprite uses value carried by this data structure.
  int skipped_sprites = 0;  ///< Skipped sprites value carried by this data structure.
  std::map<std::string, int> source_type_counts;  ///< Count of source type counts entries or events.
  std::map<std::string, int> sprite_family_counts;  ///< Count of sprite family counts entries or events.
  std::map<std::string, int> visual_kind_counts;  ///< Count of visual kind counts entries or events.

  /**
   * @brief Returns a readable dump of object visual counters.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

/**
 * @brief Resolved visual placements for runtime objects.
 */
struct ObjectVisualPlan {
  LevelSize size;  ///< Size value carried by this data structure.
  std::vector<ObjectVisualItem> items;  ///< Items value carried by this data structure.
  ObjectVisualSummary summary;  ///< Summary value carried by this data structure.

  /**
   * @brief Returns true when the object visual plan matches the map size.
   *
   * @return Validation flag.
   */
  bool IsValid() const;

  /**
   * @brief Returns a readable dump of the object visual plan.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

/**
 * @brief Builds a typed visual mapping for runtime objects.
 *
 * The pass does not change gameplay data. It maps every runtime object to a
 * typed visual sprite family or a typed fallback. It never emits object.generic.
 *
 * @param level Loaded level data with runtime objects.
 * @param error Error text populated when building fails.
 * @return Built object visual plan. Returned data is invalid on failure.
 */
ObjectVisualPlan BuildObjectVisualPlan(const LevelData& level,
                                       std::string* error);

}  // namespace sar::visual_pipeline

#endif  // SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_OBJECT_VISUAL_PLAN_H_
