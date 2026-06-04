#ifndef SHOOT_AND_RUN_CPP_SRC_APP_PROJECT_CONFIG_H_
#define SHOOT_AND_RUN_CPP_SRC_APP_PROJECT_CONFIG_H_

#include <filesystem>
#include <string>

namespace sar {

struct ProjectConfig {
  std::filesystem::path map_package_path;

  /**
   * @brief Returns a readable dump of the project configuration.
   *
   * @return String representation for logs and diagnostics.
   */
  std::string Dump() const;
};

struct ProjectConfigResult {
  bool ok = false;
  ProjectConfig config;
  std::string error;
};

/**
 * @brief Loads project configuration from a JSON file.
 *
 * The loader currently requires the `map_package_path` string field.
 * Unknown fields are ignored so the format can be extended later.
 *
 * @param config_path Path to the project configuration file.
 * @return Load result with either configuration data or an error message.
 */
ProjectConfigResult LoadProjectConfig(
    const std::filesystem::path& config_path);

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_APP_PROJECT_CONFIG_H_
