#include "../unwind.hpp"
#ifdef SENTRY_WITH_WINDOWS_UNWINDER
#include <WinBase.h>

using namespace sentry;

size_t unwinders::unwind_stack(void *addr, void **ptrs, size_t max_frames) {
    return (size_t)CaptureStackBackTrace(0, (ULONG)max_frames, ptrs, nullptr);
}

#endif
