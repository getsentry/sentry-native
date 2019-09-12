#include "base.hpp"
#ifdef SENTRY_WITH_NULL_UNWINDER

using namespace sentry;

size_t unwinders::unwind_stack(void *_addr, void **_ptrs, size_t _max_frames) {
    return 0;
}

#endif
