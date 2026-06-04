#include <iostream>
#include <string_view>

#include "app/application.h"
#include "app/app_config.h"
#include "logging/log_level.h"

namespace {

struct CliOptions {
  sar::AppConfig config;
  bool print_version = false;
};

CliOptions ParseArguments(int argc, char** argv) {
  CliOptions options;

  for (int i = 1; i < argc; ++i) {
    const std::string_view argument(argv[i]);
    constexpr std::string_view kLogLevelPrefix = "--log-level=";
    constexpr std::string_view kConfigPrefix = "--config=";

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
    }
  }

  return options;
}

}  // namespace

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
