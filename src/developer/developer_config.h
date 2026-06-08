#ifndef SHOOT_AND_RUN_CPP_SRC_DEVELOPER_DEVELOPER_CONFIG_H_
#define SHOOT_AND_RUN_CPP_SRC_DEVELOPER_DEVELOPER_CONFIG_H_

/**
 * @file src/developer/developer_config.h
 * @brief Developer-only configuration and diagnostic settings. Contains public declarations for
 * developer_config.h.
 */

#include <filesystem>
#include <string>
#include <vector>

namespace sar {

/**
 * @brief Stores developer highlight rule config data shared between runtime systems.
 */
struct DeveloperHighlightRuleConfig {
  std::string name;  ///< Human-readable name or configuration key.
  std::string regex_pattern;  ///< Regular expression used to match log text.
  std::string color;  ///< Configured color name or value.
  std::string scope = "message";  ///< Scope value carried by this data structure.
  bool case_sensitive = true;  ///< true when matching keeps case-sensitive behavior.
};

/**
 * @brief Stores developer log config data shared between runtime systems.
 */
struct DeveloperLogConfig {
  bool enabled = true;  ///< true when this configuration block or feature is enabled.
  bool color_enabled = true;  ///< Color enabled value carried by this data structure.
  bool show_execution_context = true;  ///< Boolean flag controlling show execution context.
  bool visual_pipeline_diagnostics = true;  ///< Visual pipeline diagnostics value carried by this data structure.
  bool visual_pipeline_summary = true;  ///< Visual pipeline summary value carried by this data structure.
  bool visual_pipeline_step_details = false;  ///< Visual pipeline step details value carried by this data structure.
  std::vector<DeveloperHighlightRuleConfig> highlight_rules;  ///< Highlight rules value carried by this data structure.
};

/**
 * @brief Stores developer config data shared between runtime systems.
 */
struct DeveloperConfig {
  DeveloperLogConfig log;  ///< Log value carried by this data structure.

  /**
   * @brief Returns a readable dump of developer-only configuration.
   *
   * @return String representation for diagnostics.
   */
  std::string Dump() const;
};

/**
 * @brief Stores developer config result data shared between runtime systems.
 */
struct DeveloperConfigResult {
  bool ok = false;  ///< true when the operation completed successfully.
  bool found = false;  ///< true when the optional source file was present.
  DeveloperConfig config;  ///< Config value carried by this data structure.
  std::string error;  ///< Human-readable error message when loading or validation fails.
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
