#ifndef SENTRY_CRASH_UNWIND_H_INCLUDED
#define SENTRY_CRASH_UNWIND_H_INCLUDED

#include "sentry_boot.h"

#if defined(SENTRY_PLATFORM_ANDROID)                                           \
    && defined(SENTRY_WITH_UNWINDER_LIBUNWINDSTACK)
#    include <stddef.h>
#    include <stdint.h>
#    include <sys/types.h>
#    include <ucontext.h>

#    ifdef __cplusplus
extern "C" {
#    endif

size_t sentry__crash_unwind_stack_libunwindstack(
    pid_t pid, const ucontext_t *uctx, uint64_t *ips, size_t max_frames);

#    ifdef __cplusplus
}
#    endif
#endif

#endif
