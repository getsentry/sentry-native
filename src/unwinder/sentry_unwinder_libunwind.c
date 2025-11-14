#include "sentry_boot.h"
#include "sentry_logger.h"
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <stdio.h>

typedef struct {
    uintptr_t lo, hi;
} mem_range_t;

/**
 * Looks up the memory range for a given pointer in /proc/self/maps.
 * `range` is an output parameter. Returns 0 on success.
 * Note: it is safe to use this function as long as we are running in a healthy
 * thread or in the handler thread. The function is unsafe for a signal handler.
 */
static int
find_mem_range(uintptr_t ptr, mem_range_t *range)
{
    FILE *fp = fopen("/proc/self/maps", "r");
    if (!fp) {
        return 1;
    }
    int result = 1;
    char line[512];
    while (fgets(line, sizeof line, fp)) {
        unsigned long lo, hi;
        SENTRY_INFOF("%s", line);
        if (sscanf(line, "%lx-%lx", &lo, &hi) == 2) {
            // our bounds are [lo, hi)
            if (ptr >= lo && ptr < hi) {
                range->lo = (uintptr_t)lo;
                range->hi = (uintptr_t)hi;
                SENTRY_INFOF("Found ptr (0x%" PRIx64
                             ") in the range %0x%" PRIx64 "-%0x%" PRIx64,
                    ptr, lo, hi);
                result = 0;
                break;
            }
        }
    }
    fclose(fp);
    if (result) {
        SENTRY_WARNF("Failed to find range for ptr (0x%" PRIx64 ")", ptr);
    }
    return result;
}

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
    unw_word_t ip = 0;
    if (unw_get_reg(&cursor, UNW_REG_IP, &ip) < 0) {
        return n;
    }
    if (n < max_frames) {
        ptrs[n++] = (void *)ip;
    }
    unw_word_t sp = 0;
    (void)unw_get_reg(&cursor, UNW_REG_SP, &sp);

    mem_range_t stack = { 0, 0 };
    int have_bounds = 0;
    long page_size = 0;
    if (uctx) {
        if (find_mem_range((uintptr_t)sp, &stack) == 0) {
            have_bounds = 1;
            page_size = sysconf(_SC_PAGESIZE);
            if (page_size <= 0) {
                page_size = 4096;
            }
        }
    }

    // walk the callers
    unw_word_t prev_ip = ip;
    unw_word_t prev_sp = sp;
    size_t step_idx = 0;
    while (n < max_frames) {
        SENTRY_DEBUGF("unwind: about to unw_step, step=%zu, prev_ip=%p prev_sp=%p",
            step_idx, (void*)prev_ip, (void*)prev_sp);
        if (unw_step(&cursor) <= 0) {
            SENTRY_DEBUGF("unwind: unw_step failed at step=%zu", step_idx);
            break;
        }
        SENTRY_DEBUGF("unwind: unw_step success at step=%zu", step_idx);

        // stop the walk if we fail to read IP
        if (unw_get_reg(&cursor, UNW_REG_IP, &ip) < 0) {
            SENTRY_DEBUGF("unwind: no progress at step=%zu", step_idx);
            break;
        }
        // SP is optional for progress
        (void)unw_get_reg(&cursor, UNW_REG_SP, &sp);

        // stop the walk if there is _no_ progress
        if (ip == prev_ip && sp == prev_sp) {
            break;
        }

        if (have_bounds) {
            intptr_t d_lo = (intptr_t)((uintptr_t)sp - stack.lo);
            intptr_t d_hi = (intptr_t)((uintptr_t)stack.hi - sp);
            SENTRY_DEBUGF("unwind: unw_step %zu: ip=%p, sp=%p, d_lo=%zd, d_hi=%zd", n,
                (void *)ip, (void *)sp, d_lo, d_hi);
        }

        prev_ip = ip;
        prev_sp = sp;
        ptrs[n++] = (void *)ip;
        step_idx++;
    }
    return n;
}
