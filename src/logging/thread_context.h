#ifndef SHOOT_AND_RUN_CPP_SRC_LOGGING_THREAD_CONTEXT_H_
#define SHOOT_AND_RUN_CPP_SRC_LOGGING_THREAD_CONTEXT_H_

/**
 * @file src/logging/thread_context.h
 * @brief Terminal logging, log levels, and thread context. Contains public declarations for
 * thread_context.h.
 */

#include <string_view>

namespace sar {

/**
 * @brief Assigns a human-readable name to the current thread.
 *
 * @param name Thread role or name used in log output.
 */
void SetCurrentThreadName(std::string_view name);

/**
 * @brief Returns the human-readable name of the current thread.
 *
 * @return Current thread name or a fallback value.
 */
std::string_view CurrentThreadName();

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_LOGGING_THREAD_CONTEXT_H_
