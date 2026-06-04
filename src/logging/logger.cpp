#include "logging/logger.h"

#include <charconv>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

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

int HexDigitValue(char value) {
  if (value >= '0' && value <= '9') {
    return value - '0';
  }
  if (value >= 'a' && value <= 'f') {
    return value - 'a' + 10;
  }
  if (value >= 'A' && value <= 'F') {
    return value - 'A' + 10;
  }
  return -1;
}

bool ParseHexByte(std::string_view text, std::size_t position, int* value) {
  if (position + 1 >= text.size()) {
    return false;
  }
  const int high = HexDigitValue(text[position]);
  const int low = HexDigitValue(text[position + 1]);
  if (high < 0 || low < 0) {
    return false;
  }
  *value = high * 16 + low;
  return true;
}

bool ParseAnsiColorIndex(std::string_view text, int* value) {
  constexpr std::string_view kPrefix = "ansi";
  if (!text.starts_with(kPrefix)) {
    return false;
  }

  int parsed = 0;
  const char* begin = text.data() + kPrefix.size();
  const char* end = text.data() + text.size();
  const auto result = std::from_chars(begin, end, parsed);
  if (result.ec != std::errc() || result.ptr != end || parsed < 0 ||
      parsed > 255) {
    return false;
  }

  *value = parsed;
  return true;
}

std::string ColorToAnsi(std::string_view color) {
  if (color == "red") {
    return "\033[31m";
  }
  if (color == "green") {
    return "\033[32m";
  }
  if (color == "yellow") {
    return "\033[33m";
  }
  if (color == "orange") {
    return "\033[38;5;208m";
  }
  if (color == "blue") {
    return "\033[34m";
  }
  if (color == "cyan") {
    return "\033[36m";
  }
  if (color == "gray" || color == "grey") {
    return "\033[90m";
  }
  if (color == "white") {
    return "\033[97m";
  }

  int ansi_index = 0;
  if (ParseAnsiColorIndex(color, &ansi_index)) {
    return "\033[38;5;" + std::to_string(ansi_index) + "m";
  }

  if (color.size() == 7 && color[0] == '#') {
    int red = 0;
    int green = 0;
    int blue = 0;
    if (ParseHexByte(color, 1, &red) && ParseHexByte(color, 3, &green) &&
        ParseHexByte(color, 5, &blue)) {
      return "\033[38;2;" + std::to_string(red) + ";" +
             std::to_string(green) + ";" + std::to_string(blue) + "m";
    }
  }

  return {};
}

std::string ApplySingleHighlightRule(const std::string& text,
                                     const LogHighlightRule& rule,
                                     std::string_view base_color) {
  const std::string highlight_color = ColorToAnsi(rule.color);
  if (highlight_color.empty()) {
    return text;
  }

  std::regex_constants::syntax_option_type flags = std::regex::ECMAScript;
  if (!rule.case_sensitive) {
    flags |= std::regex::icase;
  }

  try {
    const std::regex expression(rule.regex_pattern, flags);
    const std::string replacement = highlight_color + "$&" + "\033[0m" +
                                    std::string(base_color);
    return std::regex_replace(text, expression, replacement,
                              std::regex_constants::format_default);
  } catch (const std::regex_error&) {
    return text;
  }
}

std::string ApplyHighlightRules(const std::string& text,
                                const std::vector<LogHighlightRule>& rules,
                                LogHighlightScope scope,
                                std::string_view base_color) {
  std::string highlighted = text;
  for (const LogHighlightRule& rule : rules) {
    if (rule.scope != scope) {
      continue;
    }
    highlighted = ApplySingleHighlightRule(highlighted, rule, base_color);
  }
  return highlighted;
}

}  // namespace

Logger::Logger(LoggerConfig config) : config_(std::move(config)) {}

void Logger::Log(LogLevel level, std::string_view module,
                 std::string_view message) {
  if (!ShouldLog(level)) {
    return;
  }

  std::lock_guard lock(mutex_);

  const std::string base_color = config_.color_enabled
                                     ? std::string(LevelColor(level))
                                     : std::string();
  const std::string reset_color = config_.color_enabled ? "\033[0m" : "";

  std::ostringstream prefix;
  prefix << FormatTimestamp() << " [" << LogLevelName(level) << "] ";
  if (config_.show_execution_context) {
    prefix << "[pid=" << CurrentProcessId() << " tid=" << CurrentThreadId()
           << ' ' << CurrentThreadName() << "] ";
  }
  prefix << module << ": ";

  std::string message_text(message);
  std::string line = prefix.str();
  if (config_.color_enabled) {
    message_text = ApplyHighlightRules(message_text,
                                       config_.highlight_rules,
                                       LogHighlightScope::kMessage,
                                       base_color);
  }
  line += message_text;
  if (config_.color_enabled) {
    line = ApplyHighlightRules(line, config_.highlight_rules,
                               LogHighlightScope::kFullLine, base_color);
  }

  if (config_.color_enabled) {
    std::cerr << base_color;
  }
  std::cerr << line;
  if (config_.color_enabled) {
    std::cerr << reset_color;
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

void Logger::set_color_enabled(bool color_enabled) {
  config_.color_enabled = color_enabled;
}

void Logger::set_min_level(LogLevel level) { config_.min_level = level; }

void Logger::set_highlight_rules(
    std::vector<LogHighlightRule> highlight_rules) {
  config_.highlight_rules = std::move(highlight_rules);
}

LogLevel Logger::min_level() const { return config_.min_level; }

}  // namespace sar
