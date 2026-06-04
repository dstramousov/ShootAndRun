#ifndef SHOOT_AND_RUN_CPP_SRC_LOGGING_LOGGER_H_
#define SHOOT_AND_RUN_CPP_SRC_LOGGING_LOGGER_H_

#include <mutex>
#include <string_view>

#include "logging/log_level.h"

namespace sar {

struct LoggerConfig {
  LogLevel min_level = LogLevel::kInfo;
  bool color_enabled = true;
};

class Logger {
 public:
  /**
   * @brief Creates a logger with the provided runtime configuration.
   *
   * @param config Logger configuration.
   */
  explicit Logger(LoggerConfig config);

  /**
   * @brief Writes a log message if the level is enabled.
   *
   * @param level Severity level.
   * @param module Subsystem or module name.
   * @param message Human-readable log message.
   */
  void Log(LogLevel level, std::string_view module, std::string_view message);

  /**
   * @brief Writes a TRACE-level log message.
   *
   * @param module Subsystem or module name.
   * @param message Human-readable log message.
   */
  void Trace(std::string_view module, std::string_view message);

  /**
   * @brief Writes a DEBUG-level log message.
   *
   * @param module Subsystem or module name.
   * @param message Human-readable log message.
   */
  void Debug(std::string_view module, std::string_view message);

  /**
   * @brief Writes an INFO-level log message.
   *
   * @param module Subsystem or module name.
   * @param message Human-readable log message.
   */
  void Info(std::string_view module, std::string_view message);

  /**
   * @brief Writes a WARN-level log message.
   *
   * @param module Subsystem or module name.
   * @param message Human-readable log message.
   */
  void Warn(std::string_view module, std::string_view message);

  /**
   * @brief Writes an ERROR-level log message.
   *
   * @param module Subsystem or module name.
   * @param message Human-readable log message.
   */
  void Error(std::string_view module, std::string_view message);

  /**
   * @brief Writes a FATAL-level log message.
   *
   * @param module Subsystem or module name.
   * @param message Human-readable log message.
   */
  void Fatal(std::string_view module, std::string_view message);

  /**
   * @brief Checks whether a level is enabled.
   *
   * @param level Severity level to test.
   * @return true if the message should be emitted.
   */
  bool ShouldLog(LogLevel level) const;

  /**
   * @brief Updates the minimum enabled log level.
   *
   * @param level New minimum enabled log level.
   */
  void set_min_level(LogLevel level);

  /**
   * @brief Returns the minimum enabled log level.
   *
   * @return Minimum enabled log level.
   */
  LogLevel min_level() const;

 private:
  LoggerConfig config_;
  mutable std::mutex mutex_;
};

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_LOGGING_LOGGER_H_
