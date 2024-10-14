#ifndef SENTRY_OS_H_INCLUDED
#define SENTRY_OS_H_INCLUDED

#include "sentry_boot.h"

#ifdef SENTRY_PLATFORM_WINDOWS

typedef struct {
    uint32_t major;
    uint32_t minor;
    uint32_t build;
    uint32_t ubr;
} windows_version_t;

int sentry__get_kernel_version(windows_version_t *win_ver);
int sentry__get_windows_version(windows_version_t *win_ver);
void sentry__reserve_thread_stack(void);
void sentry__init_cached_functions(void);
void sentry__get_system_time(LPFILETIME filetime);

#endif

sentry_value_t sentry__get_os_context(void);

#endif
