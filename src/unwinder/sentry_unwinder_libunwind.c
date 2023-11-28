#include "sentry_boot.h"
#include "sentry_logger.h"
#include <stdio.h>
#define UNW_LOCAL_ONLY
#include <libunwind.h>

size_t
sentry__unwind_stack_libunwind(
    void *addr, const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames)
{
    if (addr) {
        return 0;
    }

    unw_cursor_t cursor;
#if 0
    if (uctx) {
        int ret = unw_init_local(&cursor, (unw_context_t *)uctx->user_context);
        if (ret != 0) {
            SENTRY_WARN("Failed to initialize libunwind with ucontext\n");
            return 0;
        }
    } else {
#endif
    unw_context_t uc;
    int ret = unw_getcontext(&uc);
    if (ret != 0) {
        SENTRY_WARN("Failed to retrieve context with libunwind\n");
        return 0;
    }

    ret = unw_init_local(&cursor, &uc);
    if (ret != 0) {
        SENTRY_WARN("Failed to initialize libunwind with local context\n");
        return 0;
    }
#if 0
    }
#endif

    size_t frame_idx = 0;
    while (unw_step(&cursor) > 0 && frame_idx < max_frames - 1) {
        unw_get_reg(&cursor, UNW_REG_IP, ptrs[frame_idx]);
        unw_word_t sp = 0;
        unw_get_reg(&cursor, UNW_REG_SP, &sp);
        printf("ip = %lx, sp = %lx\n", (long)ptrs[frame_idx], (long)sp);
        frame_idx++;
    }
    return frame_idx + 1;
}
