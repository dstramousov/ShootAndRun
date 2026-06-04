#ifndef SHOOT_AND_RUN_CPP_SRC_PLATFORM_MEMORY_INFO_H_
#define SHOOT_AND_RUN_CPP_SRC_PLATFORM_MEMORY_INFO_H_

#include <cstdint>
#include <string>

namespace sar {

struct ProcessMemoryInfo {
  bool available = false;
  std::uint64_t resident_bytes = 0;
  std::string error;
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
