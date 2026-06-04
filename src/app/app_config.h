#ifndef SHOOT_AND_RUN_CPP_SRC_APP_APP_CONFIG_H_
#define SHOOT_AND_RUN_CPP_SRC_APP_APP_CONFIG_H_

#include <string>

#include "logging/log_level.h"
#include "window/window_config.h"

namespace sar {

struct AppConfig {
  std::string app_name = "ShootAndRun";
  std::string version = "0.1.2";
  WindowConfig window;
  LogLevel log_level = LogLevel::kInfo;
  bool color_log = true;
  bool debug_overlay_enabled = true;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_APP_APP_CONFIG_H_
