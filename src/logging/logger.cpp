#include "logging/logger.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string_view>

#include "logging/thread_context.h"
#include "platform/process_info.h"

namespace sar {
namespace {

std::string FormatTimestamp() {
  const auto now = std::chrono::system_clock::now();
  const auto time = std::chrono::system_clock::to_time_t(now);
  const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch()) %
      1000;

  std::tm local_time{};
#if defined(_WIN32)
  localtime_s(&local_time, &time);
#else
  localtime_r(&time, &local_time);
#endif

  std::ostringstream stream;
  stream << std::put_time(&local_time, "%H:%M:%S") << '.' << std::setw(3)
         << std::setfill('0') << milliseconds.count();
  return stream.str();
}

std::string_view LevelColor(LogLevel level) {
  switch (level) {
    case LogLevel::kTrace:
      return "\033[90m";
    case LogLevel::kDebug:
      return "\033[37m";
    case LogLevel::kInfo:
      return "\033[0m";
    case LogLevel::kWarn:
      return "\033[33m";
    case LogLevel::kError:
      return "\033[31m";
    case LogLevel::kFatal:
      return "\033[1;31m";
  }

  return "\033[0m";
}

}  // namespace

Logger::Logger(LoggerConfig config) : config_(config) {}

void Logger::Log(LogLevel level, std::string_view module,
                 std::string_view message) {
  if (!ShouldLog(level)) {
    return;
  }

  std::lock_guard lock(mutex_);

  if (config_.color_enabled) {
    std::cerr << LevelColor(level);
  }

  std::cerr << FormatTimestamp() << " [" << LogLevelName(level) << "] ";
  if (config_.show_execution_context) {
    std::cerr << "[pid=" << CurrentProcessId() << " tid=" << CurrentThreadId()
              << ' ' << CurrentThreadName() << "] ";
  }
  std::cerr << module << ": " << message;

  if (config_.color_enabled) {
    std::cerr << "\033[0m";
  }

  std::cerr << '\n';
}

void Logger::Trace(std::string_view module, std::string_view message) {
  Log(LogLevel::kTrace, module, message);
}

void Logger::Debug(std::string_view module, std::string_view message) {
  Log(LogLevel::kDebug, module, message);
}

void Logger::Info(std::string_view module, std::string_view message) {
  Log(LogLevel::kInfo, module, message);
}

void Logger::Warn(std::string_view module, std::string_view message) {
  Log(LogLevel::kWarn, module, message);
}

void Logger::Error(std::string_view module, std::string_view message) {
  Log(LogLevel::kError, module, message);
}

void Logger::Fatal(std::string_view module, std::string_view message) {
  Log(LogLevel::kFatal, module, message);
}

bool Logger::ShouldLog(LogLevel level) const {
  return static_cast<int>(level) >= static_cast<int>(config_.min_level);
}

void Logger::set_show_execution_context(bool show_execution_context) {
  config_.show_execution_context = show_execution_context;
}

void Logger::set_min_level(LogLevel level) { config_.min_level = level; }

LogLevel Logger::min_level() const { return config_.min_level; }

}  // namespace sar
