#ifndef SENTRY_OS_H_INCLUDED
#define SENTRY_OS_H_INCLUDED

#include "sentry_boot.h"

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
void sentry__set_default_thread_stack_guarantee(void);
void sentry__init_cached_kernel32_functions(void);
void sentry__get_system_time(LPFILETIME filetime);

typedef void (*sentry__win32_abort_handler_t)(EXCEPTION_POINTERS *);
void sentry__win32_install_sigabrt_handler(
    sentry__win32_abort_handler_t handler);
void sentry__win32_restore_sigabrt_handler(void);

#endif

sentry_value_t sentry__get_os_context(void);

#endif
