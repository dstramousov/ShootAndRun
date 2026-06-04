#include "logging/thread_context.h"

#include <string>

namespace sar {
namespace {

thread_local std::string current_thread_name = "main";

}  // namespace

void SetCurrentThreadName(std::string_view name) {
  current_thread_name.assign(name.data(), name.size());
}

std::string_view CurrentThreadName() { return current_thread_name; }

}  // namespace sar
