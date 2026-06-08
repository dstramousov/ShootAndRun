/**
 * @file src/logging/thread_context.cpp
 * @brief Terminal logging, log levels, and thread context. Contains implementation for
 * thread_context.cpp.
 */

#include "logging/thread_context.h"

#include <string>

namespace sar {
namespace {

thread_local std::string current_thread_name = "main";

}  // namespace

/**
 * @brief Sets current thread name.
 */
void SetCurrentThreadName(std::string_view name) {
  current_thread_name.assign(name.data(), name.size());
}

std::string_view CurrentThreadName() { return current_thread_name; }

}  // namespace sar
