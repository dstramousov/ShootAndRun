#ifndef SHOOT_AND_RUN_CPP_SRC_LEVEL_LEVEL_LOADER_H_
#define SHOOT_AND_RUN_CPP_SRC_LEVEL_LEVEL_LOADER_H_

#include <filesystem>
#include <string>
#include <utility>

#include "level/level_data.h"

namespace sar {

/**
 * @brief Compact statistics collected while loading a map package.
 */
struct LevelPackageSummary {
  std::filesystem::path package_path;
  LevelSize size;
  int validated_runtime_grid_count = 0;
  int marker_count = 0;
  int object_count = 0;
  int place_count = 0;
  int route_count = 0;
  int elevation_transition_count = 0;
  int gameplay_zone_count = 0;
  int graph_node_count = 0;
  int graph_edge_count = 0;

  /**
   * @brief Returns a readable dump of the loaded level package summary.
   *
   * @return String representation for logs and diagnostics.
   */
  std::string Dump() const;
};

/**
 * @brief Result object returned by level package loading.
 */
struct LevelLoadResult {
  /**
   * @brief Creates an empty failed load result.
   */
  LevelLoadResult() = default;

  /**
   * @brief Creates a load result without level data.
   *
   * @param ok_value Result success flag.
   * @param summary_value Loaded package summary.
   * @param error_value Error text for failed loads.
   */
  LevelLoadResult(bool ok_value, LevelPackageSummary summary_value,
                  std::string error_value)
      : ok(ok_value),
        summary(std::move(summary_value)),
        error(std::move(error_value)) {}

  /**
   * @brief Creates a load result with level data.
   *
   * @param ok_value Result success flag.
   * @param summary_value Loaded package summary.
   * @param error_value Error text for failed loads.
   * @param level_value Loaded level data.
   */
  LevelLoadResult(bool ok_value, LevelPackageSummary summary_value,
                  std::string error_value, LevelData level_value)
      : ok(ok_value),
        summary(std::move(summary_value)),
        error(std::move(error_value)),
        level(std::move(level_value)) {}

  bool ok = false;
  LevelPackageSummary summary;
  std::string error;
  LevelData level;
};

/**
 * @brief Loads TopDownMapGen output packages into runtime level data.
 */
class LevelLoader {
 public:
  /**
   * @brief Loads and validates the basic map package files.
   *
   * This loader validates `terrain.json` and `runtime_grids.json`, checks
   * their dimensions, verifies that required runtime grids match the map
   * size, builds `LevelData` terrain cells for debug rendering, and loads
   * optional semantic layers such as markers, objects, places, routes, world
   * graph, and gameplay zones.
   *
   * @param package_path Path to a TopDownMapGen output package directory.
   * @return Load result with either summary data or an error message.
   */
  LevelLoadResult LoadBasicPackage(
      const std::filesystem::path& package_path) const;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_LEVEL_LEVEL_LOADER_H_
