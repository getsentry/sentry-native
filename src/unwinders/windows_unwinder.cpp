#include "../unwind.hpp"
#ifdef SENTRY_WITH_WINDOWS_UNWINDER
#include <WinBase.h>

using namespace sentry;

size_t unwinders::unwind_stack_windows(void *addr,
                                       const sentry_ucontext_t *uctx,
                                       void **ptrs,
                                       size_t max_frames) {
    // TODO: cannot unwind from context yet.  Use StackWalk64 here
    if (uctx || addr) {
        return 0;
    }
    return (size_t)CaptureStackBackTrace(0, (ULONG)max_frames, ptrs, nullptr);
}

#endif
