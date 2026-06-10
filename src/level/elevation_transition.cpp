/**
 * @file src/level/elevation_transition.cpp
 * @brief Generated map package data contracts and loading logic. Contains implementation for
 * elevation_transition.cpp.
 */

#include "level/elevation_transition.h"

namespace sar {

/**
 * @brief Returns elevation transition type name.
 */
const char* ElevationTransitionTypeName(ElevationTransitionType type) {
  switch (type) {
    case ElevationTransitionType::kUnknown:
      return "unknown";
    case ElevationTransitionType::kStep:
      return "step";
    case ElevationTransitionType::kRamp:
      return "ramp";
    case ElevationTransitionType::kStairs:
      return "stairs";
    case ElevationTransitionType::kHatch:
      return "hatch";
  }
  return "unknown";
}

/**
 * @brief Parses elevation transition type from external data.
 */
ElevationTransitionType ParseElevationTransitionType(const std::string& value) {
  if (value == "step" || value == "step_up" || value == "step_down" ||
      value == "ledge") {
    return ElevationTransitionType::kStep;
  }
  if (value == "ramp" || value == "slope" || value == "bridge" ||
      value == "bridge_edge") {
    return ElevationTransitionType::kRamp;
  }
  if (value == "stairs" || value == "stair" || value == "ladder") {
    return ElevationTransitionType::kStairs;
  }
  if (value == "hatch" || value == "entrance" || value == "exit" ||
      value == "bunker_entrance") {
    return ElevationTransitionType::kHatch;
  }
  return ElevationTransitionType::kUnknown;
}

}  // namespace sar
