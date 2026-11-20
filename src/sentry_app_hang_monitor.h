#ifndef SENTRY_APP_HANG_MONITOR_H_INCLUDED
#define SENTRY_APP_HANG_MONITOR_H_INCLUDED

#include "sentry_boot.h"

#include <stddef.h>
#include <stdint.h>

struct sentry_options_s;

// Interval at which the watchdog samples the heartbeat.
#if defined(SENTRY_UNITTEST)
#    define SENTRY_APP_HANG_POLL_MS 10
#else
#    define SENTRY_APP_HANG_POLL_MS 500
#endif

// Smallest timeout the watchdog can resolve meaningfully. A genuine hang fires
// somewhere in [timeout, timeout + POLL_MS) depending on the phase between the
// freeze and the next sample, so we require the timeout to be at least twice
// the poll interval to keep that sampling jitter a minority of the configured
// timeout. `sentry_options_set_app_hang_timeout` clamps non-zero values up to
// this minimum.
#define SENTRY_APP_HANG_MIN_TIMEOUT_MS (2 * SENTRY_APP_HANG_POLL_MS)

// Limits for attaching all threads to an app-hang event. Threads past the
// count limit are dropped. Threads reached after the time budget are listed
// without a stack trace.
#define SENTRY_APP_HANG_MAX_THREADS 256
#define SENTRY_APP_HANG_ALL_THREADS_BUDGET_MS 1000

// How long a stackwalk waits for a thread that does not respond (Linux and
// Android). Other threads get a short wait, so that one stuck thread cannot use
// up the budget.
#define SENTRY_APP_HANG_HUNG_THREAD_TIMEOUT_MS 1000
#define SENTRY_APP_HANG_OTHER_THREAD_TIMEOUT_MS 100

int sentry__app_hang_monitor_start(const struct sentry_options_s *options);
void sentry__app_hang_monitor_stop(void);

// Test hook: overrides the platform thread stackwalker used by the watchdog.
typedef size_t (*sentry__app_hang_stackwalk_fn)(
    uint64_t target_tid, void **ips_out, size_t max_frames);
void sentry__app_hang_monitor_set_stackwalk_fn(
    sentry__app_hang_stackwalk_fn fn);

#endif
