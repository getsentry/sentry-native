#ifndef SENTRY_UNIX_UNWINDER_LIBUNWINDSTACK_H_INCLUDED
#define SENTRY_UNIX_UNWINDER_LIBUNWINDSTACK_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "../sentry_boot.h"

size_t sentry__unwind_stack_unwindstack(
    void *addr, const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames);

#ifdef __cplusplus
}
#endif
#endif
