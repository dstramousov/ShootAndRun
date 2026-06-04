#include <string_view>

#include "app/application.h"
#include "logging/log_level.h"

namespace {

sar::AppConfig ParseArguments(int argc, char** argv) {
  sar::AppConfig config;

  for (int i = 1; i < argc; ++i) {
    const std::string_view argument(argv[i]);
    constexpr std::string_view kLogLevelPrefix = "--log-level=";

    if (argument == "--no-color") {
      config.color_log = false;
    } else if (argument.starts_with(kLogLevelPrefix)) {
      const std::string_view value = argument.substr(kLogLevelPrefix.size());
      if (const auto parsed = sar::ParseLogLevel(value); parsed.has_value()) {
        config.log_level = *parsed;
      }
    }
  }

  return config;
}

}  // namespace

int main(int argc, char** argv) {
  sar::Application application(ParseArguments(argc, argv));
  return application.Run();
}
