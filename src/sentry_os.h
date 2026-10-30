#ifndef SENTRY_OS_H_INCLUDED
#define SENTRY_OS_H_INCLUDED

#include "sentry_boot.h"

#if defined(SENTRY_PLATFORM_UNIX) && !defined(SENTRY_PLATFORM_PS)
#    include <signal.h>
#endif

#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
#    include <sys/types.h>
#    include <sys/uio.h>
#endif

#ifdef SENTRY_PLATFORM_LINUX
#    include <stdint.h>

static inline uintptr_t
sentry__ucontext_get_ip(const ucontext_t *uctx)
{
#    if defined(__i386__)
    return (uintptr_t)uctx->uc_mcontext.gregs[REG_EIP];
#    elif defined(__x86_64__)
    return (uintptr_t)uctx->uc_mcontext.gregs[REG_RIP];
#    elif defined(__arm__)
    return (uintptr_t)uctx->uc_mcontext.arm_pc;
#    elif defined(__aarch64__)
    return (uintptr_t)uctx->uc_mcontext.pc;
#    elif defined(__riscv) && __riscv_xlen == 64
    return (uintptr_t)uctx->uc_mcontext.__gregs[REG_PC];
#    else
    (void)uctx;
    return 0;
#    endif
}

static inline uintptr_t
sentry__ucontext_get_sp(const ucontext_t *uctx)
{
#    if defined(__i386__)
    return (uintptr_t)uctx->uc_mcontext.gregs[REG_ESP];
#    elif defined(__x86_64__)
    return (uintptr_t)uctx->uc_mcontext.gregs[REG_RSP];
#    elif defined(__arm__)
    return (uintptr_t)uctx->uc_mcontext.arm_sp;
#    elif defined(__aarch64__)
    return (uintptr_t)uctx->uc_mcontext.sp;
#    elif defined(__riscv) && __riscv_xlen == 64
    return (uintptr_t)uctx->uc_mcontext.__gregs[REG_SP];
#    else
    (void)uctx;
    return 0;
#    endif
}
#endif

#ifdef SENTRY_PLATFORM_WINDOWS

typedef struct {
    uint32_t major;
    uint32_t minor;
    uint32_t build;
    uint32_t ubr;
} windows_version_t;

int sentry__get_kernel_version(windows_version_t *win_ver);
int sentry__get_windows_version(windows_version_t *win_ver);

#    if !defined(SENTRY_PLATFORM_XBOX)
sentry_value_t sentry__get_wine_context(void);
#    endif

void sentry__set_default_thread_stack_guarantee(void);
void sentry__init_cached_kernel32_functions(void);
void sentry__get_system_time(LPFILETIME filetime);

typedef void (*sentry__win32_abort_handler_t)(EXCEPTION_POINTERS *);
void sentry__win32_install_sigabrt_handler(
    sentry__win32_abort_handler_t handler);
void sentry__win32_restore_sigabrt_handler(void);

#endif

#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
ssize_t sentry__process_vm_readv(pid_t pid, const struct iovec *local_iov,
    unsigned long local_iov_count, const struct iovec *remote_iov,
    unsigned long remote_iov_count, unsigned long flags);
#endif

sentry_value_t sentry__get_os_context(void);

sentry_value_t sentry__build_registers(const sentry_ucontext_t *uctx);

#if defined(SENTRY_PLATFORM_UNIX)
const char *sentry__signal_to_string(int signum);
#endif

#if defined(SENTRY_PLATFORM_UNIX) && !defined(SENTRY_PLATFORM_PS)
/**
 * Sets up an alternate signal stack for the current thread if one isn't
 * already configured. The allocated stack is stored in `out_stack` so it
 * can be freed later via `sentry__teardown_sigaltstack()`.
 * Returns 0 on success or -1 on failure.
 */
int sentry__setup_sigaltstack(
    stack_t *out_stack, size_t size, const char *context);

/**
 * Tears down a signal stack previously set up via
 * `sentry__setup_sigaltstack()`.
 */
void sentry__teardown_sigaltstack(stack_t *sig_stack);
#endif

#endif
