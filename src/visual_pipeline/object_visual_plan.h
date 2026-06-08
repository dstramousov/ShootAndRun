#ifndef SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_OBJECT_VISUAL_PLAN_H_
#define SHOOT_AND_RUN_CPP_SRC_VISUAL_PIPELINE_OBJECT_VISUAL_PLAN_H_

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
  std::string id;
  std::string source_type;
  std::string source_family;
  std::string sprite_family;
  std::string sprite_id;
  std::string draw_layer;
  ObjectVisualKind kind = ObjectVisualKind::kUnknown;
  int x = 0;
  int y = 0;
  int width = 1;
  int height = 1;
  int sort_y = 0;
};

/**
 * @brief Counters produced by the runtime object visual planning pass.
 */
struct ObjectVisualSummary {
  int source_object_count = 0;
  int mapped_object_count = 0;
  int typed_fallback_count = 0;
  int generic_object_count = 0;
  int missing_sprite_uses = 0;
  int skipped_sprites = 0;
  std::map<std::string, int> source_type_counts;
  std::map<std::string, int> sprite_family_counts;
  std::map<std::string, int> visual_kind_counts;

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
  LevelSize size;
  std::vector<ObjectVisualItem> items;
  ObjectVisualSummary summary;

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
