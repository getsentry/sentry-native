#ifndef SENTRY_UNWINDERS_BASE_HPP_INCLUDED
#define SENTRY_UNWINDERS_BASE_HPP_INCLUDED

#ifdef _WIN32
#define SENTRY_WITH_WINDOWS_UNWINDER
#elif defined(__APPLE__) || (defined(__linux__) && !defined(__ANDROID__))
#define SENTRY_WITH_BACKTRACE_UNWINDER
#else
#define SENTRY_WITH_NULL_UNWINDER
#endif

#include <functional>
#include "internal.hpp"

namespace sentry {
namespace unwinders {

size_t unwind_stack(void *addr, void **ptrs, size_t max_frames);

}  // namespace unwinders
}  // namespace sentry

#endif
