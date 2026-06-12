#ifndef SHOOT_AND_RUN_CPP_SRC_LEVEL_LEVEL_LOADER_H_
#define SHOOT_AND_RUN_CPP_SRC_LEVEL_LEVEL_LOADER_H_

/**
 * @file src/level/level_loader.h
 * @brief Generated map package data contracts and loading logic. Contains public declarations
 * for level_loader.h.
 */

#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "level/level_data.h"

namespace sar {


/**
 * @brief Detailed one-time validation report for a loaded map package.
 *
 * The report is built during package loading and is intended for integration
 * diagnostics between the map generator and the runtime client. It does not
 * affect per-frame gameplay performance.
 */
struct LevelPackageValidationReport {
  int total_tiles = 0;  ///< Total tile count in the loaded map.
  int walkable_tiles = 0;  ///< Count of tiles that allow movement.
  int collision_tiles = 0;  ///< Count of tiles that block physical movement.
  int projectile_block_tiles = 0;  ///< Count of tiles that block projectiles.
  int vision_block_tiles = 0;  ///< Count of tiles that block vision.
  int cover_tiles = 0;  ///< Count of tiles with cover.
  int concealment_tiles = 0;  ///< Count of tiles with concealment.
  int min_elevation = 0;  ///< Minimum elevation found in the height grid.
  int max_elevation = 0;  ///< Maximum elevation found in the height grid.
  std::map<int, int> elevation_histogram;  ///< Tile counts grouped by elevation.
  std::map<std::string, int> terrain_histogram;  ///< Tile counts grouped by terrain id.
  std::map<std::string, int> transition_histogram;  ///< Transition counts grouped by type.
  int synthetic_transition_count = 0;  ///< Count of runtime-generated fallback transitions.
  int transition_endpoint_mismatch_count = 0;  ///< Count of transitions whose declared endpoint elevations differ from height_grid.
  int transition_large_delta_count = 0;  ///< Count of transitions crossing more than one elevation level.
  int negative_region_count = 0;  ///< Count of connected regions below elevation 0.
  int open_negative_region_count = 0;  ///< Count of negative regions with at least one walkable boundary entry.
  int closed_negative_region_count = 0;  ///< Count of negative regions without walkable boundary entries.
  int negative_region_tile_count = 0;  ///< Count of all tiles below elevation 0.
  std::vector<std::string> warnings;  ///< Non-fatal validation warnings.

  /**
   * @brief Returns a readable multi-line validation report.
   *
   * @return Multi-line text suitable for startup logs.
   */
  std::string DumpMultiline() const;
};

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
  int synthetic_elevation_transition_count = 0;  ///< Count of generated fallback elevation transitions.
  int gameplay_zone_count = 0;  ///< Count of gameplay zone count entries or events.
  int graph_node_count = 0;  ///< Count of graph node count entries or events.
  int graph_edge_count = 0;  ///< Count of graph edge count entries or events.
  LevelPackageValidationReport validation_report;  ///< One-time map validation diagnostics.

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
