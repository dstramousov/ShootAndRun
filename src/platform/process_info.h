#ifndef SHOOT_AND_RUN_CPP_SRC_PLATFORM_PROCESS_INFO_H_
#define SHOOT_AND_RUN_CPP_SRC_PLATFORM_PROCESS_INFO_H_

/**
 * @file src/platform/process_info.h
 * @brief Platform-specific process, terminal, and memory utilities. Contains public
 * declarations for process_info.h.
 */

#include <cstdint>

namespace sar {

/**
 * @brief Returns the current process identifier.
 *
 * @return Platform process identifier as an unsigned integer.
 */
std::uint64_t CurrentProcessId();

/**
 * @brief Returns the current thread identifier.
 *
 * @return Platform thread identifier as an unsigned integer.
 */
std::uint64_t CurrentThreadId();

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_PLATFORM_PROCESS_INFO_H_
