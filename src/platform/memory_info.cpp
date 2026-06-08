/**
 * @file src/platform/memory_info.cpp
 * @brief Platform-specific process, terminal, and memory utilities. Contains implementation for
 * memory_info.cpp.
 */

#include "platform/memory_info.h"

#include <iomanip>
#include <sstream>

#if defined(__linux__)
#include <fstream>
#include <limits>
#include <unistd.h>
#endif

namespace sar {

/**
 * @brief Reads current process memory info.
 */
ProcessMemoryInfo ReadCurrentProcessMemoryInfo() {
#if defined(__linux__)
  std::ifstream statm("/proc/self/statm");
  if (!statm) {
    return {false, 0, "failed to open /proc/self/statm"};
  }

  unsigned long total_pages = 0;
  unsigned long resident_pages = 0;
  statm >> total_pages >> resident_pages;
  if (!statm) {
    return {false, 0, "failed to parse /proc/self/statm"};
  }

  const long page_size = sysconf(_SC_PAGESIZE);
  if (page_size <= 0) {
    return {false, 0, "failed to read system page size"};
  }

  const auto page_size_bytes = static_cast<std::uint64_t>(page_size);
  if (resident_pages > std::numeric_limits<std::uint64_t>::max() /
                           page_size_bytes) {
    return {false, 0, "resident memory value overflow"};
  }

  return {true, static_cast<std::uint64_t>(resident_pages) * page_size_bytes,
          {}};
#else
  return {false, 0, "process memory info is not supported on this platform"};
#endif
}

/**
 * @brief Executes the format megabytes operation.
 */
std::string FormatMegabytes(std::uint64_t bytes) {
  constexpr double kBytesPerMegabyte = 1024.0 * 1024.0;
  const double megabytes = static_cast<double>(bytes) / kBytesPerMegabyte;

  std::ostringstream stream;
  stream << std::fixed << std::setprecision(1) << megabytes << " MB";
  return stream.str();
}

}  // namespace sar
