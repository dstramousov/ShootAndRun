#ifndef SHOOT_AND_RUN_CPP_SRC_DEVELOPER_DEVELOPER_CONFIG_H_
#define SHOOT_AND_RUN_CPP_SRC_DEVELOPER_DEVELOPER_CONFIG_H_

#include <filesystem>
#include <string>
#include <vector>

namespace sar {

struct DeveloperHighlightRuleConfig {
  std::string name;
  std::string regex_pattern;
  std::string color;
  std::string scope = "message";
  bool case_sensitive = true;
};

struct DeveloperLogConfig {
  bool enabled = true;
  bool color_enabled = true;
  bool show_execution_context = true;
  bool visual_pipeline_diagnostics = true;
  std::vector<DeveloperHighlightRuleConfig> highlight_rules;
};

struct DeveloperConfig {
  DeveloperLogConfig log;

  /**
   * @brief Returns a readable dump of developer-only configuration.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

struct DeveloperConfigResult {
  bool ok = false;
  bool found = false;
  DeveloperConfig config;
  std::string error;
};

/**
 * @brief Loads developer-only configuration from a JSON file.
 *
 * Missing files are not treated as fatal. The caller receives `found=false` and
 * safe defaults remain available. Unknown fields are ignored so the config can
 * be extended without breaking older builds.
 *
 * @param config_path Path to the developer configuration file.
 * @return Load result with either configuration data, defaults, or an error.
 */
DeveloperConfigResult LoadDeveloperConfig(
    const std::filesystem::path& config_path);

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_DEVELOPER_DEVELOPER_CONFIG_H_
