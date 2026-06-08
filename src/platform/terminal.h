#ifndef SHOOT_AND_RUN_CPP_SRC_PLATFORM_TERMINAL_H_
#define SHOOT_AND_RUN_CPP_SRC_PLATFORM_TERMINAL_H_

/**
 * @file src/platform/terminal.h
 * @brief Platform-specific process, terminal, and memory utilities. Contains public
 * declarations for terminal.h.
 */

namespace sar {

/**
 * @brief Checks whether standard error is connected to a terminal.
 *
 * @return true if standard error is a terminal.
 */
bool IsStderrTerminal();

/**
 * @brief Checks whether color output was disabled by environment.
 *
 * @return true if NO_COLOR is present in the environment.
 */
bool IsNoColorEnvironmentSet();

/**
 * @brief Resolves whether terminal colors should be used.
 *
 * @param requested User-requested color preference.
 * @return true if color output should be enabled.
 */
bool ShouldUseTerminalColor(bool requested);

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_PLATFORM_TERMINAL_H_
