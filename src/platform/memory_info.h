#ifndef SHOOT_AND_RUN_CPP_SRC_PLATFORM_MEMORY_INFO_H_
#define SHOOT_AND_RUN_CPP_SRC_PLATFORM_MEMORY_INFO_H_

/**
 * @file src/platform/memory_info.h
 * @brief Platform-specific process, terminal, and memory utilities. Contains public
 * declarations for memory_info.h.
 */

#include <cstdint>
#include <string>

namespace sar {

/**
 * @brief Stores process memory info data shared between runtime systems.
 */
struct ProcessMemoryInfo {
  bool available = false;  ///< Available value carried by this data structure.
  std::uint64_t resident_bytes = 0;  ///< Resident bytes value carried by this data structure.
  std::string error;  ///< Human-readable error message when loading or validation fails.
};

/**
 * @brief Reads resident memory usage for the current process.
 *
 * The value is the resident set size, meaning the amount of physical memory
 * currently mapped by this process. If the platform does not expose this
 * information, the returned structure contains `available = false` and a
 * human-readable error message.
 *
 * @return Current process memory information.
 */
ProcessMemoryInfo ReadCurrentProcessMemoryInfo();

/**
 * @brief Formats a byte count as a human-readable megabyte string.
 *
 * @param bytes Number of bytes to format.
 * @return Formatted string, for example `84.2 MB`.
 */
std::string FormatMegabytes(std::uint64_t bytes);

}  // namespace sar

#endif  // SHOOT_AND_RUN_CPP_SRC_PLATFORM_MEMORY_INFO_H_
