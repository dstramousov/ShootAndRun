#ifndef SHOOT_AND_RUN_CPP_SRC_APP_APP_CONFIG_H_
#define SHOOT_AND_RUN_CPP_SRC_APP_APP_CONFIG_H_

/**
 * @file src/app/app_config.h
 * @brief Application configuration, lifecycle, startup, and runtime orchestration. Contains
 * public declarations for app_config.h.
 */

#include <filesystem>
#include <string>
#include <string_view>

#include "logging/log_level.h"
#include "window/window_config.h"

#ifndef SAR_APP_VERSION
#define SAR_APP_VERSION "0.1.61-dev"
#endif

namespace sar {

/**
 * @brief Renderer backend selected for the runtime session.
 */
enum class RuntimeRendererMode {
  kRenderer2D,
  kRenderer3D,
};

/**
 * @brief Returns the stable command-line name of a runtime renderer mode.
 *
 * @param mode Runtime renderer mode.
 * @return Stable lowercase renderer name.
 */
constexpr std::string_view RuntimeRendererModeName(RuntimeRendererMode mode) {
  switch (mode) {
    case RuntimeRendererMode::kRenderer2D:
      return "2d";
    case RuntimeRendererMode::kRenderer3D:
      return "3d";
  }
  return "2d";
}

/**
 * @brief Startup options resolved before the main application object is created.
 */
struct AppConfig {
  std::string app_name = "ShootAndRun";  ///< App name value carried by this data structure.
  std::string version = SAR_APP_VERSION;  ///< Version value carried by this data structure.
  int target_fps = 60;  ///< Target fps value carried by this data structure.
  WindowConfig window;  ///< Window value carried by this data structure.
  std::filesystem::path project_config_path = "config/app_config.json";  ///< Filesystem path used by this configuration or data object.
  std::filesystem::path developer_config_path = "config/developer_log_config.json";  ///< Filesystem path used by this configuration or data object.
  RuntimeRendererMode renderer_mode = RuntimeRendererMode::kRenderer2D;  ///< Renderer mode value carried by this data structure.
  LogLevel log_level = LogLevel::kInfo;  ///< Log level value carried by this data structure.
  bool color_log = true;  ///< Color log value carried by this data structure.
  bool debug_overlay_enabled = true;  ///< Debug overlay enabled value carried by this data structure.
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_APP_APP_CONFIG_H_
