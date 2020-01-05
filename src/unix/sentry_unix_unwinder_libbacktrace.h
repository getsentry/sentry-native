#ifndef SENTRY_UNIX_UNWINDER_LIBBACKTRACE_H_INCLUDED
#define SENTRY_UNIX_UNWINDER_LIBBACKTRACE_H_INCLUDED

#include "../sentry_boot.h"

size_t sentry__unwind_stack_backtrace(
    void *addr, const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames);

#endif
