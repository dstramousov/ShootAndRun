/**
 * @file src/platform/terminal.cpp
 * @brief Platform-specific process, terminal, and memory utilities. Contains implementation for
 * terminal.cpp.
 */

#include "platform/terminal.h"

#include <cstdio>
#include <cstdlib>

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

namespace sar {

/**
 * @brief Checks whether stderr terminal is true.
 */
bool IsStderrTerminal() {
#if defined(_WIN32)
  return _isatty(_fileno(stderr)) != 0;
#else
  return isatty(fileno(stderr)) != 0;
#endif
}

bool IsNoColorEnvironmentSet() { return std::getenv("NO_COLOR") != nullptr; }

/**
 * @brief Returns the color used for should use terminal.
 */
bool ShouldUseTerminalColor(bool requested) {
  return requested && IsStderrTerminal() && !IsNoColorEnvironmentSet();
}

}  // namespace sar
