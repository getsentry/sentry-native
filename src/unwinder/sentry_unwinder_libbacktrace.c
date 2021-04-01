#include "sentry_boot.h"

#if defined(SENTRY_PLATFORM_LINUX) && defined(__GLIBC__)
#    include <execinfo.h>
#endif

#ifndef __has_builtin
#    define __has_builtin(x) 0
#endif

size_t
sentry__unwind_stack_libbacktrace(
    void *addr, const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames)
{
    if (addr) {
#if defined(SENTRY_PLATFORM_MACOS) && __has_builtin(__builtin_available)
        if (__builtin_available(macOS 10.14, *))
            return (size_t)backtrace_from_fp(addr, ptrs, (int)max_frames);
#endif
        return 0;
    } else if (uctx) {
        return 0;
    }
#if defined(SENTRY_PLATFORM_LINUX) && defined(__GLIBC__)
    return (size_t)backtrace(ptrs, (int)max_frames);
#else
    (void)ptrs;
    (void)max_frames;
    return 0;
#endif
}
