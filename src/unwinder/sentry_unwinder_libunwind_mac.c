#include "sentry_boot.h"
#include "sentry_logger.h"
#include <libunwind.h>

// a very cheap pointer validation for starters
static bool
valid_ptr(uintptr_t p)
{
    return p && (p % sizeof(uintptr_t) == 0);
}

size_t
fp_walk_from_uctx(const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames)
{
    size_t frame_idx = 0;
    struct __darwin_mcontext64 *mctx = uctx->user_context->uc_mcontext;
#if defined(__arm64__)
    uintptr_t pc = (uintptr_t)mctx->__ss.__pc;
    uintptr_t fp = (uintptr_t)mctx->__ss.__fp;
    uintptr_t lr = (uintptr_t)mctx->__ss.__lr;

    // top frame: adjust pc−1 so it symbolizes inside the function
    if (pc) {
        ptrs[frame_idx++] = (void *)(pc - 1);
    }

    // next frame is from saved LR at current FP record
    if (lr) {
        ptrs[frame_idx++] = (void *)(lr - 1);
    }

    for (size_t i = 0; i < max_frames; ++i) {
        if (!valid_ptr(fp)) {
            break;
        }

        // arm64 frame record layout: [prev_fp, saved_lr] at fp and fp+8
        uintptr_t *record = (uintptr_t *)fp;
        uintptr_t next_fp = record[0];
        uintptr_t ret_addr = record[1];
        if (!valid_ptr(next_fp) || !ret_addr) {
            break;
        }

        ptrs[frame_idx++] = (void *)(ret_addr - 1);
        if (next_fp <= fp) {
            break; // prevent loops
        }
        fp = next_fp;
    }
#elif defined(__x86_64__)
    uintptr_t ip = (uintptr_t)mctx->__ss.__rip;
    uintptr_t bp = (uintptr_t)mctx->__ss.__rbp;

    // top frame: adjust ip−1 so it symbolizes inside the function
    if (ip) {
        ptrs[frame_idx++] = (void *)(ip - 1);
    }

    for (size_t i = 0; i < max_frames; ++i) {
        if (!valid_ptr(bp)) {
            break;
        }
        // x86_64 frame record layout: [prev_rbp, saved_retaddr] at bp and bp+8
        uintptr_t *record = (uintptr_t *)bp;
        uintptr_t next_bp = record[0];
        uintptr_t ret_addr = record[1];
        if (!valid_ptr(next_bp) || !ret_addr) {
            break;
        }
        ptrs[frame_idx++] = (void *)(ret_addr - 1);
        if (next_bp <= bp) {
            break;
        }
        bp = next_bp;
    }
#else
#    error "Unsupported CPU architecture for macOS stackwalker"
#endif
    return frame_idx + 1;
}

size_t
sentry__unwind_stack_libunwind_mac(
    void *addr, const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames)
{
    if (addr) {
        // we don't support stack walks from arbitrary addresses
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
    size_t frame_idx = 0;
    while (unw_step(&cursor) > 0 && frame_idx < max_frames - 1) {
        unw_word_t ip = 0;
        SENTRY_INFOF("ip: %p", ip);
        unw_get_reg(&cursor, UNW_REG_IP, &ip);
#if defined(__arm64__)
        // Strip pointer authentication, for some reason ptrauth_strip() not
        // working
        // https://developer.apple.com/documentation/security/preparing_your_app_to_work_with_pointer_authentication
        ip &= 0x7fffffffffffull;
#endif
        ptrs[frame_idx] = (void *)ip;
        frame_idx++;
    }

    return frame_idx + 1;
}
