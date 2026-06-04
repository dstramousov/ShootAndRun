#include "level/terrain_type.h"

namespace sar {

TerrainType TerrainTypeFromString(std::string_view value) {
  if (value == "open_ground" || value == "grass" || value == "clearing" ||
      value == "ground" || value == "dirt") {
    return TerrainType::kOpenGround;
  }
  if (value == "forest" || value == "tree" || value == "trees" ||
      value == "tree_blocker" || value == "dense_forest") {
    return TerrainType::kForest;
  }
  if (value == "road" || value == "path" || value == "trail" ||
      value == "dirt_path") {
    return TerrainType::kRoad;
  }
  if (value == "swamp" || value == "mud" || value == "bog") {
    return TerrainType::kSwamp;
  }
  if (value == "ruins" || value == "ruin" || value == "ruin_floor" ||
      value == "stone_floor" || value == "rubble") {
    return TerrainType::kRuins;
  }
  if (value == "water" || value == "water_slow" || value == "pond" ||
      value == "lake") {
    return TerrainType::kWater;
  }
  if (value == "wall" || value == "stone_wall" || value == "ruin_wall") {
    return TerrainType::kWall;
  }
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
