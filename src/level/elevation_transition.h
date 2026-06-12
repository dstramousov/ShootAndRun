#ifndef SHOOT_AND_RUN_CPP_SRC_LEVEL_ELEVATION_TRANSITION_H_
#define SHOOT_AND_RUN_CPP_SRC_LEVEL_ELEVATION_TRANSITION_H_

/**
 * @file src/level/elevation_transition.h
 * @brief Generated map package data contracts and loading logic. Contains public declarations
 * for elevation_transition.h.
 */

#include <cstdint>
#include <string>

namespace sar {

/**
 * @brief Semantic type of a movement transition between elevation levels.
 */
enum class ElevationTransitionType {
  kUnknown,
  kStep,
  kRamp,
  kStairs,
  kHatch,
};

/**
 * @brief One explicit allowed transition between two neighboring elevation tiles.
 */
struct ElevationTransition {
  std::string id;  ///< Stable identifier loaded from source data or configuration.
  ElevationTransitionType type = ElevationTransitionType::kUnknown;  ///< Semantic type loaded from source data or configuration.
  int from_x = -1;  ///< Tile, screen, or world coordinate for from x.
  int from_y = -1;  ///< Tile, screen, or world coordinate for from y.
  int to_x = -1;  ///< Tile, screen, or world coordinate for to x.
  int to_y = -1;  ///< Tile, screen, or world coordinate for to y.
  std::int8_t from_elevation = 0;  ///< From elevation value carried by this data structure.
  std::int8_t to_elevation = 0;  ///< To elevation value carried by this data structure.
  bool bidirectional = true;  ///< Bidirectional value carried by this data structure.
  bool synthetic = false;  ///< True when the runtime generated this transition as a fallback.
  std::string source_id;  ///< Source marker or object identifier for synthetic transitions.
};

/**
 * @brief Returns the stable lowercase name of an elevation transition type.
 *
 * @param type Elevation transition type.
 * @return Stable lowercase transition type name.
 */
const char* ElevationTransitionTypeName(ElevationTransitionType type);

/**
 * @brief Parses a stable elevation transition type name.
 *
 * @param value Source transition type name.
 * @return Parsed transition type or kUnknown for unsupported values.
 */
ElevationTransitionType ParseElevationTransitionType(const std::string& value);

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_LEVEL_ELEVATION_TRANSITION_H_
