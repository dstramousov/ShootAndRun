/**
 * @file src/main.cpp
 * @brief ShootAndRun runtime helpers. Contains implementation for main.cpp.
 */

#include <iostream>
#include <string_view>

#include "app/application.h"
#include "app/app_config.h"
#include "logging/log_level.h"

namespace {

/**
 * @brief Stores cli options data shared between runtime systems.
 */
struct CliOptions {
  sar::AppConfig config;
  bool print_version = false;
};

/**
 * @brief Parses command-line arguments into application startup options.
 */
CliOptions ParseArguments(int argc, char** argv) {
  CliOptions options;

  for (int i = 1; i < argc; ++i) {
    const std::string_view argument(argv[i]);
    constexpr std::string_view kLogLevelPrefix = "--log-level=";
    constexpr std::string_view kConfigPrefix = "--config=";
    constexpr std::string_view kDeveloperConfigPrefix = "--developer-config=";
    constexpr std::string_view kRendererPrefix = "--renderer=";

    if (argument == "--version") {
      options.print_version = true;
    } else if (argument == "--no-color") {
      options.config.color_log = false;
    } else if (argument.starts_with(kLogLevelPrefix)) {
      const std::string_view value = argument.substr(kLogLevelPrefix.size());
      if (const auto parsed = sar::ParseLogLevel(value); parsed.has_value()) {
        options.config.log_level = *parsed;
      }
    } else if (argument.starts_with(kConfigPrefix)) {
      options.config.project_config_path = argument.substr(kConfigPrefix.size());
    } else if (argument.starts_with(kRendererPrefix)) {
      const std::string_view value = argument.substr(kRendererPrefix.size());
      if (value == "3d") {
        options.config.renderer_mode = sar::RuntimeRendererMode::kRenderer3D;
      } else if (value == "2d") {
        options.config.renderer_mode = sar::RuntimeRendererMode::kRenderer2D;
      }
    } else if (argument.starts_with(kDeveloperConfigPrefix)) {
      options.config.developer_config_path =
          argument.substr(kDeveloperConfigPrefix.size());
    }
  }

  return options;
}

}  // namespace

/**
 * @brief Creates the application and returns its process exit code.
 */
int main(int argc, char** argv) {
  const CliOptions options = ParseArguments(argc, argv);
  if (options.print_version) {
    std::cout << options.config.app_name << " v" << options.config.version
              << '\n';
    return 0;
  }

  sar::Application application(options.config);
  return application.Run();
}
