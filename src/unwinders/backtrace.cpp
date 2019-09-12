#include "base.hpp"
#ifdef SENTRY_WITH_BACKTRACE_UNWINDER
#include <execinfo.h>

using namespace sentry;

size_t unwinders::unwind_stack(void *addr, void **ptrs, size_t max_frames) {
    if (addr) {
        #if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_14
        return backtrace_from_fp(addr, ptrs, max_frames);
        #endif
        return 0;
    } else {
        return backtrace(ptrs, max_frames);
    }
}

#endif
