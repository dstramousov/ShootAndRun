#include "level/terrain_type.h"

namespace sar {

TerrainType TerrainTypeFromString(std::string_view value) {
  if (value == "open_ground") return TerrainType::kOpenGround;
  if (value == "forest") return TerrainType::kForest;
  if (value == "road") return TerrainType::kRoad;
  if (value == "swamp") return TerrainType::kSwamp;
  if (value == "ruins") return TerrainType::kRuins;
  if (value == "water") return TerrainType::kWater;
  if (value == "wall") return TerrainType::kWall;
  return TerrainType::kUnknown;
}

std::string_view TerrainTypeToString(TerrainType terrain) {
  switch (terrain) {
    case TerrainType::kOpenGround:
      return "open_ground";
    case TerrainType::kForest:
      return "forest";
    case TerrainType::kRoad:
      return "road";
    case TerrainType::kSwamp:
      return "swamp";
    case TerrainType::kRuins:
      return "ruins";
    case TerrainType::kWater:
      return "water";
    case TerrainType::kWall:
      return "wall";
    case TerrainType::kUnknown:
      return "unknown";
  }

  return "unknown";
}

}  // namespace sar
