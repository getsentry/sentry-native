#include "sentry_boot.h"
#include "sentry_logger.h"
#include "sentry_unwinder.h"
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <stdio.h>

/**
 * Looks up the memory range for a given pointer in /proc/self/maps.
 * `range` is an output parameter. Returns `true` if a range was found.
 * Note: it is safe to use this function as long as we are running in a healthy
 * thread or in the handler thread. The function is unsafe for a signal handler.
 */
static bool
find_mem_range(uintptr_t ptr, mem_range_t *range)
{
    bool found = false;
    FILE *fp = fopen("/proc/self/maps", "r");
    if (!fp) {
        return found;
    }
    char line[512];
    while (fgets(line, sizeof line, fp)) {
        unsigned long lo, hi;
        if (sscanf(line, "%lx-%lx", &lo, &hi) == 2) {
            // our bounds are [lo, hi)
            if (ptr >= lo && ptr < hi) {
                range->lo = (uintptr_t)lo;
                range->hi = (uintptr_t)hi;
                found = true;
                break;
            }
        }
    }
    fclose(fp);
    return found;
}

size_t
sentry__unwind_stack_libunwind(
    void *addr, const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames)
{
    if (addr) {
        return 0;
    }

    unw_cursor_t cursor;
    unw_context_t uc;
    if (uctx) {
        int ret = unw_init_local2(&cursor, (unw_context_t *)uctx->user_context,
            UNW_INIT_SIGNAL_FRAME);
        if (ret != 0) {
            SENTRY_WARN("Failed to initialize libunwind with ucontext");
            return 0;
        }
    } else {
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
    // get the first frame and stack pointer
    unw_word_t ip = 0;
    if (unw_get_reg(&cursor, UNW_REG_IP, &ip) < 0) {
        return n;
    }
    if (n < max_frames) {
        ptrs[n++] = (void *)ip;
    }
    unw_word_t sp = 0;
    (void)unw_get_reg(&cursor, UNW_REG_SP, &sp);

    // ensure we have a valid stack pointer otherwise we only send the top frame
    mem_range_t stack = { 0, 0 };
    if (uctx && !find_mem_range((uintptr_t)sp, &stack)) {
        SENTRY_WARNF("unwinder: SP (%p) is in unmapped memory likely due to "
                     "stack overflow",
            (void *)sp);
        return n;
    }

    // walk the callers
    unw_word_t prev_ip = ip;
    unw_word_t prev_sp = sp;
    while (n < max_frames && unw_step(&cursor) > 0) {
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
