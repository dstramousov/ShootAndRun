#include "platform/process_info.h"

#include <functional>
#include <thread>

#if defined(_WIN32)
#include <processthreadsapi.h>
#elif defined(__linux__)
#include <sys/syscall.h>
#include <unistd.h>
#elif defined(__APPLE__)
#include <pthread.h>
#include <unistd.h>
#else
#include <unistd.h>
#endif

namespace sar {

std::uint64_t CurrentProcessId() {
#if defined(_WIN32)
  return static_cast<std::uint64_t>(GetCurrentProcessId());
#else
  return static_cast<std::uint64_t>(getpid());
#endif
}

std::uint64_t CurrentThreadId() {
#if defined(_WIN32)
  return static_cast<std::uint64_t>(GetCurrentThreadId());
#elif defined(__linux__)
  return static_cast<std::uint64_t>(syscall(SYS_gettid));
#elif defined(__APPLE__)
  std::uint64_t thread_id = 0;
  pthread_threadid_np(nullptr, &thread_id);
  return thread_id;
#else
  return static_cast<std::uint64_t>(
      std::hash<std::thread::id>{}(std::this_thread::get_id()));
#endif
}

}  // namespace sar
