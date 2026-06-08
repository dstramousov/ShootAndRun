#ifndef SHOOT_AND_RUN_CPP_SRC_LEVEL_ELEVATION_TRANSITION_H_
#define SHOOT_AND_RUN_CPP_SRC_LEVEL_ELEVATION_TRANSITION_H_

#include <cstdint>
#include <string>

namespace sar {

enum class ElevationTransitionType {
  kUnknown,
  kStep,
  kRamp,
  kStairs,
  kHatch,
};

struct ElevationTransition {
  std::string id;
  ElevationTransitionType type = ElevationTransitionType::kUnknown;
  int from_x = -1;
  int from_y = -1;
  int to_x = -1;
  int to_y = -1;
  std::int8_t from_elevation = 0;
  std::int8_t to_elevation = 0;
  bool bidirectional = true;
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
