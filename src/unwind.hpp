#ifndef SENTRY_UNWINDERS_BASE_HPP_INCLUDED
#define SENTRY_UNWINDERS_BASE_HPP_INCLUDED

/* if the null unwinder is explicitly selected we disable all unwinders */
#ifndef SENTRY_WITH_NULL_UNWINDER
#ifdef _WIN32
#define SENTRY_WITH_WINDOWS_UNWINDER
#elif defined(__ANDROID__)
#define SENTRY_WITH_LIBUNWINDSTACK_UNWINDER
#elif defined(__APPLE__) || defined(__linux__)
#define SENTRY_WITH_BACKTRACE_UNWINDER
#else
#define SENTRY_WITH_NULL_UNWINDER
#endif
#endif

#include <functional>
#include "internal.hpp"

namespace sentry {
size_t unwind_stack(void *addr, void **ptrs, size_t max_frames);

namespace unwinders {
#ifdef SENTRY_WITH_WINDOWS_UNWINDER
size_t unwind_stack_windows(void *addr, void **ptrs, size_t max_frames);
#endif
#ifdef SENTRY_WITH_BACKTRACE_UNWINDER
size_t unwind_stack_backtrace(void *addr, void **ptrs, size_t max_frames);
#endif
#ifdef SENTRY_WITH_LIBUNWINDSTACK_UNWINDER
size_t unwind_stack_libunwindstack(void *addr, void **ptrs, size_t max_frames);
#endif
}  // namespace unwinders
}  // namespace sentry

#endif
