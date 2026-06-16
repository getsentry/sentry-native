#ifndef SENTRY_APP_HANG_MONITOR_H_INCLUDED
#define SENTRY_APP_HANG_MONITOR_H_INCLUDED

#include <stddef.h>
#include <stdint.h>

struct sentry_options_s;

int sentry__app_hang_monitor_start(const struct sentry_options_s *options);
void sentry__app_hang_monitor_stop(void);

typedef size_t (*sentry__app_hang_thread_sampler_fn)(
    uint64_t target_tid, void **ips_out, size_t max_frames);
void sentry__app_hang_monitor_set_thread_sampler(
    sentry__app_hang_thread_sampler_fn fn);

#endif
