#ifndef SHOOT_AND_RUN_CPP_SRC_LEVEL_TERRAIN_TYPE_H_
#define SHOOT_AND_RUN_CPP_SRC_LEVEL_TERRAIN_TYPE_H_

#include <string_view>

namespace sar {

/**
 * @brief Normalized terrain type used by the runtime and renderers.
 */
enum class TerrainType {
  kUnknown,
  kOpenGround,
  kForest,
  kRoad,
  kSwamp,
  kRuins,
  kWater,
  kWall,
};

/**
 * @brief Converts a generator terrain identifier to an enum value.
 *
 * @param value Terrain identifier from JSON.
 * @return Terrain type enum value.
 */
TerrainType TerrainTypeFromString(std::string_view value);

/**
 * @brief Converts a terrain enum value to a stable identifier.
 *
 * @param terrain Terrain type enum value.
 * @return Stable terrain identifier.
 */
std::string_view TerrainTypeToString(TerrainType terrain);

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_LEVEL_TERRAIN_TYPE_H_
