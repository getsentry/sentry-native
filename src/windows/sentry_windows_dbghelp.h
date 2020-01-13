#ifndef SENTRY_WINDOWS_DBGHELP_H_INCLUDED
#define SENTRY_WINDOWS_DBGHELP_H_INCLUDED

#include "../sentry_boot.h"

#include "../sentry_symbolizer.h"

bool sentry__symbolize_dbghelp(
    void *addr, void (*func)(const sentry_frame_info_t *, void *), void *data);
size_t sentry__unwind_stack_dbghelp(
    void *addr, const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames);

#endif
