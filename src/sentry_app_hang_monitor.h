#ifndef SENTRY_APP_HANG_MONITOR_H_INCLUDED
#define SENTRY_APP_HANG_MONITOR_H_INCLUDED

#include "sentry_boot.h"

#include <stddef.h>
#include <stdint.h>

struct sentry_options_s;

int sentry__app_hang_monitor_start(const struct sentry_options_s *options);
void sentry__app_hang_monitor_stop(void);

// Test hook: overrides the platform thread stackwalker used by the watchdog.
typedef size_t (*sentry__app_hang_stackwalk_fn)(
    uint64_t target_tid, void **ips_out, size_t max_frames);
void sentry__app_hang_monitor_set_stackwalk_fn(
    sentry__app_hang_stackwalk_fn fn);

#endif
