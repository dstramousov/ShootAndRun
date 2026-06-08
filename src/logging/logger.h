#ifndef SHOOT_AND_RUN_CPP_SRC_LOGGING_LOGGER_H_
#define SHOOT_AND_RUN_CPP_SRC_LOGGING_LOGGER_H_

/**
 * @file src/logging/logger.h
 * @brief Terminal logging, log levels, and thread context. Contains public declarations for
 * logger.h.
 */

#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include "logging/log_level.h"

namespace sar {

/**
 * @brief Defines the supported log highlight scope values.
 */
enum class LogHighlightScope {
  kMessage,
  kFullLine,
};

/**
 * @brief Stores log highlight rule data shared between runtime systems.
 */
struct LogHighlightRule {
  std::string name;  ///< Human-readable name or configuration key.
  std::string regex_pattern;  ///< Regular expression used to match log text.
  std::string color;  ///< Configured color name or value.
  LogHighlightScope scope = LogHighlightScope::kMessage;  ///< Scope value carried by this data structure.
  bool case_sensitive = true;  ///< true when matching keeps case-sensitive behavior.
};

/**
 * @brief Stores logger config data shared between runtime systems.
 */
struct LoggerConfig {
  LogLevel min_level = LogLevel::kInfo;  ///< Min level value carried by this data structure.
  bool color_enabled = true;  ///< Color enabled value carried by this data structure.
  bool show_execution_context = true;  ///< Boolean flag controlling show execution context.
  std::vector<LogHighlightRule> highlight_rules;  ///< Highlight rules value carried by this data structure.
};

/**
 * @brief Owns the logger behavior and its runtime state.
 */
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
   * @brief Updates execution context visibility in log lines.
   *
   * @param show_execution_context true to print PID, TID, and thread name.
   */
  void set_show_execution_context(bool show_execution_context);

  /**
   * @brief Updates ANSI color output visibility.
   *
   * @param color_enabled true to emit ANSI color codes.
   */
  void set_color_enabled(bool color_enabled);

  /**
   * @brief Updates the minimum enabled log level.
   *
   * @param level New minimum enabled log level.
   */
  void set_min_level(LogLevel level);

  /**
   * @brief Replaces log highlighting rules.
   *
   * @param highlight_rules Rules applied to emitted log lines.
   */
  void set_highlight_rules(std::vector<LogHighlightRule> highlight_rules);

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
