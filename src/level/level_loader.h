#ifndef SHOOT_AND_RUN_CPP_SRC_LEVEL_LEVEL_LOADER_H_
#define SHOOT_AND_RUN_CPP_SRC_LEVEL_LEVEL_LOADER_H_

#include <filesystem>
#include <string>

#include "level/level_data.h"

namespace sar {

struct LevelPackageSummary {
  std::filesystem::path package_path;
  LevelSize size;
  int validated_runtime_grid_count = 0;

  /**
   * @brief Returns a readable dump of the loaded level package summary.
   *
   * @return String representation for logs and diagnostics.
   */
  std::string Dump() const;
};

struct LevelLoadResult {
  bool ok = false;
  LevelPackageSummary summary;
  std::string error;
};

class LevelLoader {
 public:
  /**
   * @brief Loads and validates the basic map package files.
   *
   * This MVP loader validates `terrain.json` and `runtime_grids.json`, checks
   * their dimensions, and verifies that required runtime grids match the map
   * size. It does not build renderable tile data yet.
   *
   * @param package_path Path to a TopDownMapGen output package directory.
   * @return Load result with either summary data or an error message.
   */
  LevelLoadResult LoadBasicPackage(
      const std::filesystem::path& package_path) const;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_LEVEL_LEVEL_LOADER_H_
