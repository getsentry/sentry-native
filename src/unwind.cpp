#include "unwind.hpp"

using namespace sentry;
using namespace unwinders;

size_t sentry::unwind_stack(void *addr,
                            const sentry_ucontext_t *uctx,
                            void **ptrs,
                            size_t max_frames) {
#define TRY_UNWINDER(Func)                              \
    do {                                                \
        size_t rv = Func(addr, uctx, ptrs, max_frames); \
        if (rv > 0) {                                   \
            return rv;                                  \
        }                                               \
    } while (0)
#ifdef SENTRY_WITH_WINDOWS_UNWINDER
    TRY_UNWINDER(unwind_stack_windows);
#endif
#ifdef SENTRY_WITH_BACKTRACE_UNWINDER
    TRY_UNWINDER(unwind_stack_backtrace);
#endif
#ifdef SENTRY_WITH_LIBUNWINDSTACK_UNWINDER
    TRY_UNWINDER(unwind_stack_libunwindstack);
#endif
#undef TRY_UNWINDER
    return 0;
}
