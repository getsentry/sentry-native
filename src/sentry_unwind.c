#include "sentry_boot.h"

#ifdef SENTRY_PLATFORM_DARWIN
#    include "unix/sentry_unix_unwinder_libbacktrace.h"
#    define HAVE_LIBBACKTRACE
#endif
#ifdef SENTRY_PLATFORM_WINDOWS
#    include "windows/sentry_windows_dbghelp.h"
#    define HAVE_DBGHELP
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
#ifdef HAVE_LIBBACKTRACE
    TRY_UNWINDER(sentry__unwind_stack_backtrace);
#endif
#ifdef HAVE_DBGHELP
    TRY_UNWINDER(sentry__unwind_stack_dbghelp);
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
