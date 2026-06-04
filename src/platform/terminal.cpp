#include "platform/terminal.h"

#include <cstdio>
#include <cstdlib>

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

namespace sar {

bool IsStderrTerminal() {
#if defined(_WIN32)
  return _isatty(_fileno(stderr)) != 0;
#else
  return isatty(fileno(stderr)) != 0;
#endif
}

bool IsNoColorEnvironmentSet() { return std::getenv("NO_COLOR") != nullptr; }

bool ShouldUseTerminalColor(bool requested) {
  return requested && IsStderrTerminal() && !IsNoColorEnvironmentSet();
}

}  // namespace sar
