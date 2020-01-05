#include "sentry_boot.h"

#if SENTRY_PLATFORM == SENTRY_PLATFORM_DARWIN
#    include "unix/sentry_unix_unwinder_libbacktrace.h"
#endif

#define TRY_UNWINDER(Func)                                                     \
    do {                                                                       \
        size_t rv = Func(addr, uctx, ptrs, max_frames);                        \
        if (rv > 0) {                                                          \
            return rv;                                                         \
        }                                                                      \
    } while (0)

static size_t
unwind_stack(
    void *addr, const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames)
{
#if SENTRY_PLATFORM == SENTRY_PLATFORM_DARWIN
    TRY_UNWINDER(sentry__unwind_stack_backtrace);
#endif
    return 0;
}

size_t
sentry_unwind_stack(void *addr, void **stacktrace_out, size_t max_len)
{
    return unwind_stack(addr, NULL, stacktrace_out, max_len);
}

size_t
sentry_unwind_stack_from_ucontext(
    const sentry_ucontext_t *uctx, void **stacktrace_out, size_t max_len)
{
    return unwind_stack(NULL, uctx, stacktrace_out, max_len);
}
