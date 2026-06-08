/**
 * @file src/logging/log_level.cpp
 * @brief Terminal logging, log levels, and thread context. Contains implementation for
 * log_level.cpp.
 */

#include "logging/log_level.h"

#include <algorithm>
#include <string>

namespace sar {

/**
 * @brief Writes diagnostics for level name.
 */
std::string_view LogLevelName(LogLevel level) {
  switch (level) {
    case LogLevel::kTrace:
      return "TRACE";
    case LogLevel::kDebug:
      return "DEBUG";
    case LogLevel::kInfo:
      return "INFO";
    case LogLevel::kWarn:
      return "WARN";
    case LogLevel::kError:
      return "ERROR";
    case LogLevel::kFatal:
      return "FATAL";
  }

  return "UNKNOWN";
}

/**
 * @brief Parses log level from external data.
 */
std::optional<LogLevel> ParseLogLevel(std::string_view value) {
  std::string normalized(value);
  std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                 [](unsigned char character) {
                   return static_cast<char>(std::tolower(character));
                 });

  if (normalized == "trace") return LogLevel::kTrace;
  if (normalized == "debug") return LogLevel::kDebug;
  if (normalized == "info") return LogLevel::kInfo;
  if (normalized == "warn" || normalized == "warning") return LogLevel::kWarn;
  if (normalized == "error") return LogLevel::kError;
  if (normalized == "fatal") return LogLevel::kFatal;

  return std::nullopt;
}

}  // namespace sar
