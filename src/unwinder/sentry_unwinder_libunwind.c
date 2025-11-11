#include "sentry_boot.h"
#include "sentry_logger.h"
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
    if (uctx) {
        int ret = unw_init_local2(&cursor, (unw_context_t *)uctx->user_context,
            UNW_INIT_SIGNAL_FRAME);
        if (ret != 0) {
            SENTRY_WARN("Failed to initialize libunwind with ucontext");
            return 0;
        }
    } else {
        unw_context_t uc;
#ifdef __clang__
// This pragma is required to build with Werror on ARM64 Ubuntu
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored                                           \
        "-Wgnu-statement-expression-from-macro-expansion"
#endif
        int ret = unw_getcontext(&uc);
#ifdef __clang__
#    pragma clang diagnostic pop
#endif
        if (ret != 0) {
            SENTRY_WARN("Failed to retrieve context with libunwind");
            return 0;
        }

        ret = unw_init_local(&cursor, &uc);
        if (ret != 0) {
            SENTRY_WARN("Failed to initialize libunwind with local context");
            return 0;
        }
    }

    size_t n = 0;
    // get the first frame
    if (n < max_frames) {
        unw_word_t ip = 0;
        if (unw_get_reg(&cursor, UNW_REG_IP, &ip) < 0) {
            return n;
        }
        ptrs[n++] = (void *)ip;
    }
    // walk the callers
    unw_word_t prev_ip = (unw_word_t)ptrs[0];
    unw_word_t prev_sp = 0;
    (void)unw_get_reg(&cursor, UNW_REG_SP, &prev_sp);

    while (n < max_frames && unw_step(&cursor) > 0) {
        unw_word_t ip = 0, sp = 0;
        // stop the walk if we fail to read IP
        if (unw_get_reg(&cursor, UNW_REG_IP, &ip) < 0) {
            break;
        }
        // SP is optional for progress
        (void)unw_get_reg(&cursor, UNW_REG_SP, &sp);

        // stop the walk if there is _no_ progress
        if (ip == prev_ip && sp == prev_sp) {
            break;
        }
        prev_ip = ip;
        prev_sp = sp;
        ptrs[n++] = (void *)ip;
    }
    return n;
}
