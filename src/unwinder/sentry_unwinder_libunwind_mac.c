#include "sentry_boot.h"
#include "sentry_logger.h"
#include <libunwind.h>

// a very cheap pointer validation for starters
static bool
valid_ptr(uintptr_t p)
{
    return p && (p % sizeof(uintptr_t) == 0);
}

/**
 * This does the same frame-pointer walk for arm64 and x86_64, with the only
 * difference being which registers value is used as frame-pointer (fp vs rbp)
 */
static void
fp_walk(uintptr_t fp, size_t *n, void **ptrs, size_t max_frames)
{
    while (*n < max_frames) {
        if (!valid_ptr(fp)) {
            break;
        }

        // arm64 frame record layout: [prev_fp, saved_lr] at fp and fp+8
        // x86_64 frame record layout: [prev_rbp, saved_retaddr] at bp and bp+8
        const uintptr_t *record = (uintptr_t *)fp;
        const uintptr_t next_fp = record[0];
        const uintptr_t ret_addr = record[1];
        if (!valid_ptr(next_fp) || !ret_addr) {
            break;
        }

        ptrs[(*n)++] = (void *)(ret_addr - 1);
        if (next_fp <= fp) {
            break; // prevent loops
        }
        fp = next_fp;
    }
}

static size_t
fp_walk_from_uctx(const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames)
{
    size_t n = 0;
    struct __darwin_mcontext64 *mctx = uctx->user_context->uc_mcontext;
#if defined(__arm64__)
    uintptr_t pc = (uintptr_t)mctx->__ss.__pc;
    uintptr_t fp = (uintptr_t)mctx->__ss.__fp;
    uintptr_t lr = (uintptr_t)mctx->__ss.__lr;

    // top frame: adjust pc−1 so it symbolizes inside the function
    if (pc && n < max_frames) {
        ptrs[n++] = (void *)(pc - 1);
    }

    // next frame is from saved LR at current FP record
    if (lr && n < max_frames) {
        ptrs[n++] = (void *)(lr - 1);
    }

    fp_walk(fp, &n, ptrs, max_frames);
#elif defined(__x86_64__)
    uintptr_t ip = (uintptr_t)mctx->__ss.__rip;
    uintptr_t bp = (uintptr_t)mctx->__ss.__rbp;

    // top frame: adjust ip−1 so it symbolizes inside the function
    if (ip && n < max_frames) {
        ptrs[n++] = (void *)(ip - 1);
    }

    fp_walk(bp, &n, ptrs, max_frames);
#else
#    error "Unsupported CPU architecture for macOS stackwalker"
#endif
    return n;
}

size_t
sentry__unwind_stack_libunwind_mac(
    void *addr, const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames)
{
    if (addr) {
        size_t n = 0;
        fp_walk((uintptr_t)addr, &n, ptrs, max_frames);
        return 0;
    }

    if (uctx) {
        return fp_walk_from_uctx(uctx, ptrs, max_frames);
    }

    // fall back on the system `libunwind` for stack-traces "from call-site"
    unw_context_t uc;
    int ret = unw_getcontext(&uc);
    if (ret != 0) {
        SENTRY_WARN("Failed to retrieve context with libunwind");
        return 0;
    }

    unw_cursor_t cursor;
    ret = unw_init_local(&cursor, &uc);
    if (ret != 0) {
        SENTRY_WARN("Failed to initialize libunwind with local context");
        return 0;
    }
    size_t n = 0;
    // get the first frame
    if (n < max_frames) {
        unw_word_t ip = 0;
        if (unw_get_reg(&cursor, UNW_REG_IP, &ip) >= 0) {
#if defined(__arm64__)
            // Strip pointer authentication, for some reason ptrauth_strip() not
            // working
            // https://developer.apple.com/documentation/security/preparing_your_app_to_work_with_pointer_authentication
            ip &= 0x7fffffffffffull;
#endif
            ptrs[n++] = (void *)ip;
        } else {
            return 0;
        }
    }
    // walk the callers
    unw_word_t prev_ip = (uintptr_t)ptrs[0];
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
#if defined(__arm64__)
        // Strip pointer authentication, for some reason ptrauth_strip() not
        // working
        // https://developer.apple.com/documentation/security/preparing_your_app_to_work_with_pointer_authentication
        ip &= 0x7fffffffffffull;
#endif
        prev_ip = ip;
        prev_sp = sp;
        ptrs[n++] = (void *)ip;
    }

    return n;
}
