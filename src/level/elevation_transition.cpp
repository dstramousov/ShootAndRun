#include "level/elevation_transition.h"

namespace sar {

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

ElevationTransitionType ParseElevationTransitionType(const std::string& value) {
  if (value == "step" || value == "step_up" || value == "ledge") {
    return ElevationTransitionType::kStep;
  }
  if (value == "ramp" || value == "slope") {
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
