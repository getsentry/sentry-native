#include "../unwind.hpp"
#ifdef SENTRY_WITH_BACKTRACE_UNWINDER
#include <execinfo.h>

using namespace sentry;

size_t unwinders::unwind_stack_backtrace(void *addr,
                                         void **ptrs,
                                         size_t max_frames) {
    if (addr) {
#ifdef MAC_OS_X_VERSION_10_14
        return backtrace_from_fp(addr, ptrs, max_frames);
#endif
        return 0;
    } else {
        return backtrace(ptrs, max_frames);
    }
}

#endif
