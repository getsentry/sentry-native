#include "sentry_unix_unwinder_libbacktrace.h"
#ifndef SENTRY_PLATFORM_ANDROID
#include <execinfo.h>
#endif

size_t
sentry__unwind_stack_backtrace(
    void *addr, const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames)
{
#ifdef SENTRY_PLATFORM_ANDROID
    return 0;
#else
    if (addr) {
#ifdef MAC_OS_X_VERSION_10_14
        return backtrace_from_fp(addr, ptrs, max_frames);
#endif
        return 0;
    } else if (uctx) {
        return 0;
    } else {
        return backtrace(ptrs, max_frames);
    }
#endif
}
