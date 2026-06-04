#ifndef SHOOT_AND_RUN_CPP_SRC_LEVEL_LEVEL_LOADER_H_
#define SHOOT_AND_RUN_CPP_SRC_LEVEL_LEVEL_LOADER_H_

#include <filesystem>
#include <string>
#include <utility>

#include "level/level_data.h"

namespace sar {

struct LevelPackageSummary {
  std::filesystem::path package_path;
  LevelSize size;
  int validated_runtime_grid_count = 0;
  int marker_count = 0;

  /**
   * @brief Returns a readable dump of the loaded level package summary.
   *
   * @return String representation for logs and diagnostics.
   */
  std::string Dump() const;
};

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

class LevelLoader {
 public:
  /**
   * @brief Loads and validates the basic map package files.
   *
   * This MVP loader validates `terrain.json` and `runtime_grids.json`, checks
   * their dimensions, verifies that required runtime grids match the map
   * size, builds a basic `LevelData` terrain cell array for debug
   * rendering, and loads optional gameplay markers for debug overlays and
   * camera centering.
   *
   * @param package_path Path to a TopDownMapGen output package directory.
   * @return Load result with either summary data or an error message.
   */
  LevelLoadResult LoadBasicPackage(
      const std::filesystem::path& package_path) const;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_LEVEL_LEVEL_LOADER_H_
