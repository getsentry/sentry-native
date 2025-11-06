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
sentry__unwind_stack_libunwind_mac(
    void *addr, const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames)
{
    if (addr) {
        return 0;
    }
    size_t frame_idx = 0;

    if (uctx) {
        // TODO: this is a working ARM64 from ucontext unwinder using FP that
        //       doesn't have the issues we see backtrace() (which isn't even
        //       signal-safe) but also the system provided libunwind() on macOS
        //       - clean this up
        //       - implement an x86_64 ucontext unwinder

        struct __darwin_mcontext64 *mctx = uctx->user_context->uc_mcontext;
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
            uintptr_t retaddr = record[1];
            if (!valid_ptr(next_fp) || !retaddr) {
                break;
            }

            ptrs[frame_idx++] = (void *)(retaddr - 1);
            if (next_fp <= fp) {
                break; // prevent loops
            }
            fp = next_fp;
        }
    } else {
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
    }

    return frame_idx + 1;
}
