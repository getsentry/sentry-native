#include "sentry_boot.h"

// XXX: Make into a CMake check
// XXX: IBM i PASE offers libbacktrace in libutil, but not available in AIX
#if defined(SENTRY_PLATFORM_DARWIN) || defined(__GLIBC__) || defined(__PASE__)
#    define HAS_EXECINFO_H
#endif

#if defined(SENTRY_PLATFORM_MACOS) && defined(MAC_OS_X_VERSION_10_6)
#    define HAS_LIBUNWIND
#endif

#ifdef HAS_EXECINFO_H
#    include <execinfo.h>
#endif

#ifdef HAS_LIBUNWIND
#    include <libunwind.h>

#    define ARRAY_SIZE(a) sizeof(a) / sizeof(a[0])

static size_t
sentry__unwind_stack_libbacktrace_libnuwind(
    const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames)
{
    unw_context_t ctx;
    if (unw_getcontext(&ctx))
        return 0;

    unw_cursor_t cursor;
    if (unw_init_local(&cursor, &ctx))
        return 0;

#    if defined(__x86_64__)
    if (unw_set_reg(&cursor, UNW_X86_64_RAX,
            uctx->user_context->uc_mcontext->__ss.__rax)
        || unw_set_reg(&cursor, UNW_X86_64_RDX,
            uctx->user_context->uc_mcontext->__ss.__rdx)
        || unw_set_reg(&cursor, UNW_X86_64_RCX,
            uctx->user_context->uc_mcontext->__ss.__rcx)
        || unw_set_reg(&cursor, UNW_X86_64_RBX,
            uctx->user_context->uc_mcontext->__ss.__rbx)
        || unw_set_reg(&cursor, UNW_X86_64_RSI,
            uctx->user_context->uc_mcontext->__ss.__rsi)
        || unw_set_reg(&cursor, UNW_X86_64_RDI,
            uctx->user_context->uc_mcontext->__ss.__rdi)
        || unw_set_reg(&cursor, UNW_X86_64_RBP,
            uctx->user_context->uc_mcontext->__ss.__rbp)
        || unw_set_reg(&cursor, UNW_X86_64_RSP,
            uctx->user_context->uc_mcontext->__ss.__rsp)
        || unw_set_reg(
            &cursor, UNW_X86_64_R8, uctx->user_context->uc_mcontext->__ss.__r8)
        || unw_set_reg(
            &cursor, UNW_X86_64_R9, uctx->user_context->uc_mcontext->__ss.__r9)
        || unw_set_reg(&cursor, UNW_X86_64_R10,
            uctx->user_context->uc_mcontext->__ss.__r10)
        || unw_set_reg(&cursor, UNW_X86_64_R11,
            uctx->user_context->uc_mcontext->__ss.__r11)
        || unw_set_reg(&cursor, UNW_X86_64_R12,
            uctx->user_context->uc_mcontext->__ss.__r12)
        || unw_set_reg(&cursor, UNW_X86_64_R13,
            uctx->user_context->uc_mcontext->__ss.__r13)
        || unw_set_reg(&cursor, UNW_X86_64_R14,
            uctx->user_context->uc_mcontext->__ss.__r14)
        || unw_set_reg(&cursor, UNW_X86_64_R15,
            uctx->user_context->uc_mcontext->__ss.__r15)
        || unw_set_reg(
            &cursor, UNW_REG_IP, uctx->user_context->uc_mcontext->__ss.__rip))
        return 0;

#    elif defined(__arm64__)
    for (size_t i = 0;
        i < ARRAY_SIZE(uctx->user_context->uc_mcontext->__ss.__x); ++i) {
        if (unw_set_reg(&cursor, UNW_AARCH64_X0 + i,
                uctx->user_context->uc_mcontext->__ss.__x[i])) {
            return 0;
        }
    }

    if (unw_set_reg(
            &cursor, UNW_AARCH64_FP, uctx->user_context->uc_mcontext->__ss.__fp)
        || unw_set_reg(
            &cursor, UNW_AARCH64_LR, uctx->user_context->uc_mcontext->__ss.__lr)
        || unw_set_reg(
            &cursor, UNW_AARCH64_SP, uctx->user_context->uc_mcontext->__ss.__sp)
        || unw_set_reg(
            &cursor, UNW_REG_IP, uctx->user_context->uc_mcontext->__ss.__pc))
        return 0;

#    endif

    size_t n = 0;
    for (int err = 1; err >= 0 && n < max_frames; err = unw_step(&cursor)) {
        unw_word_t ip;
        if (unw_get_reg(&cursor, UNW_REG_IP, &ip)) {
            break;
        }

#    if defined(__arm64__)
        // Strip pointer authentication, for some reason ptrauth_strip() not
        // working
        // https://developer.apple.com/documentation/security/preparing_your_app_to_work_with_pointer_authentication
        ip &= 0x7fffffffffffull;
#    endif

        ptrs[n++] = (void *)ip;

        // last frame
        if (err == 0) {
            break;
        }
    }

    return n;
}
#endif

size_t
sentry__unwind_stack_libbacktrace(
    void *addr, const sentry_ucontext_t *uctx, void **ptrs, size_t max_frames)
{
    if (addr) {
#if defined(SENTRY_PLATFORM_MACOS) && defined(MAC_OS_X_VERSION_10_14)          \
    && __has_builtin(__builtin_available)
        if (__builtin_available(macOS 10.14, *)) {
            return (size_t)backtrace_from_fp(addr, ptrs, (int)max_frames);
        }
#endif
        return 0;
    } else if (uctx) {
#ifdef HAS_LIBUNWIND
        return sentry__unwind_stack_libbacktrace_libnuwind(
            uctx, ptrs, max_frames);
#else
        return 0;
#endif
    }
#ifdef HAS_EXECINFO_H
    return (size_t)backtrace(ptrs, (int)max_frames);
#else
    (void)ptrs;
    (void)max_frames;
    return 0;
#endif
}
