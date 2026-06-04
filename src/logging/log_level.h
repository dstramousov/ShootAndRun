#ifndef SHOOT_AND_RUN_CPP_SRC_LOGGING_LOG_LEVEL_H_
#define SHOOT_AND_RUN_CPP_SRC_LOGGING_LOG_LEVEL_H_

#include <optional>
#include <string_view>

namespace sar {

enum class LogLevel {
  kTrace = 0,
  kDebug = 1,
  kInfo = 2,
  kWarn = 3,
  kError = 4,
  kFatal = 5,
};

/**
 * @brief Converts a log level to its stable text representation.
 *
 * @param level Log level value.
 * @return Uppercase log level name.
 */
std::string_view LogLevelName(LogLevel level);

/**
 * @brief Parses a log level from a command-line value.
 *
 * The parser accepts lowercase and uppercase level names.
 *
 * @param value Text value to parse.
 * @return Parsed log level or std::nullopt if the value is unknown.
 */
std::optional<LogLevel> ParseLogLevel(std::string_view value);

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_LOGGING_LOG_LEVEL_H_
