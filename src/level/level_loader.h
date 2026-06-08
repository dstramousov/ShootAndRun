#ifndef SHOOT_AND_RUN_CPP_SRC_LEVEL_LEVEL_LOADER_H_
#define SHOOT_AND_RUN_CPP_SRC_LEVEL_LEVEL_LOADER_H_

/**
 * @file src/level/level_loader.h
 * @brief Generated map package data contracts and loading logic. Contains public declarations
 * for level_loader.h.
 */

#include <filesystem>
#include <string>
#include <utility>

#include "level/level_data.h"

namespace sar {

/**
 * @brief Compact statistics collected while loading a map package.
 */
struct LevelPackageSummary {
  std::filesystem::path package_path;  ///< Filesystem path used by this configuration or data object.
  LevelSize size;  ///< Size value carried by this data structure.
  int validated_runtime_grid_count = 0;  ///< Count of validated runtime grid count entries or events.
  int marker_count = 0;  ///< Count of marker count entries or events.
  int object_count = 0;  ///< Count of object count entries or events.
  int place_count = 0;  ///< Count of place count entries or events.
  int route_count = 0;  ///< Count of route count entries or events.
  int elevation_transition_count = 0;  ///< Count of elevation transition count entries or events.
  int gameplay_zone_count = 0;  ///< Count of gameplay zone count entries or events.
  int graph_node_count = 0;  ///< Count of graph node count entries or events.
  int graph_edge_count = 0;  ///< Count of graph edge count entries or events.

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

  bool ok = false;  ///< true when the operation completed successfully.
  LevelPackageSummary summary;  ///< Summary value carried by this data structure.
  std::string error;  ///< Human-readable error message when loading or validation fails.
  LevelData level;  ///< Level value carried by this data structure.
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
