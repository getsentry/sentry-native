#include "sentry_app_hang_monitor.h"

#include "sentry_app_hang_latch.h"
#include "sentry_core.h"
#include "sentry_logger.h"
#include "sentry_options.h"
#include "sentry_sync.h"
#include "sentry_thread_stackwalk.h"

#include <stdio.h>
#include <string.h>

sentry_value_t
sentry__app_hang_make_event(void **ips, size_t frame_count, uint64_t freeze_ms)
{
    char value_buf[128];
    snprintf(value_buf, sizeof(value_buf), "App hung for at least %llu ms.",
        (unsigned long long)freeze_ms);

    sentry_value_t event = sentry_value_new_event();
    sentry_value_set_by_key(event, "level", sentry_value_new_string("error"));
    sentry_value_set_by_key(
        event, "message", sentry_value_new_string(value_buf));

    sentry_value_t exc = sentry_value_new_exception("AppHang", value_buf);

    sentry_value_t mechanism = sentry_value_new_object();
    sentry_value_set_by_key(
        mechanism, "type", sentry_value_new_string("AppHang"));
    sentry_value_set_by_key(mechanism, "handled", sentry_value_new_bool(true));
    sentry_value_set_by_key(
        mechanism, "synthetic", sentry_value_new_bool(true));
    sentry_value_set_by_key(exc, "mechanism", mechanism);

    sentry_value_set_stacktrace(exc, ips, frame_count);

    sentry_event_add_exception(event, exc);
    return event;
}

static sentry__app_hang_stackwalk_fn g_stackwalk_override = NULL;

void
sentry__app_hang_monitor_set_stackwalk_fn(sentry__app_hang_stackwalk_fn fn)
{
    g_stackwalk_override = fn;
}

// Everything below is the watchdog machinery, which only makes sense where a
// platform thread stackwalker exists. On other platforms the public start/stop
// are no-ops (see below) and these would be unused (which -Werror rejects), so
// they are compiled out entirely.
#if SENTRY_HAS_THREAD_STACKWALK

static bool g_running = false;
static volatile long g_stop = 0;
static sentry_threadid_t g_thread;
static sentry_mutex_t g_wait_mutex = SENTRY__MUTEX_INIT;
static sentry_cond_t g_wait_cond;
static uint64_t g_timeout_ms = 0;

#    define SENTRY_APP_HANG_POLL_MS 500

static size_t
stackwalk_thread(uint64_t tid, void **ips, size_t max)
{
    // A test installs an override.
    return g_stackwalk_override != NULL
        ? g_stackwalk_override(tid, ips, max)
        : sentry__thread_stackwalk(tid, ips, max);
}

static void
app_hang_capture(uint64_t hang_time_ms, uint64_t tid)
{
    void *ips[SENTRY_APP_HANG_MAX_FRAMES];
    size_t n = stackwalk_thread(tid, ips, SENTRY_APP_HANG_MAX_FRAMES);
    if (n == 0) {
        SENTRY_DEBUG("app-hang: no frames sampled, skipping event");
        return;
    }
    sentry_value_t event = sentry__app_hang_make_event(ips, n, hang_time_ms);
    sentry__capture_event(event, NULL);
}

SENTRY_THREAD_FN
worker(void *arg)
{
    (void)arg;
    uint64_t last_fired_hb = 0;
    while (!sentry__atomic_fetch(&g_stop)) {
        sentry__mutex_lock(&g_wait_mutex);
        sentry__cond_wait_timeout(
            &g_wait_cond, &g_wait_mutex, SENTRY_APP_HANG_POLL_MS);
        sentry__mutex_unlock(&g_wait_mutex);

        if (sentry__atomic_fetch(&g_stop)) {
            break;
        }

        sentry_app_hang_latch_t latch;
        sentry__app_hang_latch_read(&latch);
        uint64_t now = sentry__app_hang_now_ms();
        if (sentry__app_hang_should_capture(
                latch.last_heartbeat_ms, now, g_timeout_ms, last_fired_hb)) {
            app_hang_capture(now - latch.last_heartbeat_ms, latch.target_tid);
            last_fired_hb = latch.last_heartbeat_ms;
        }
    }
    return 0;
}

int
sentry__app_hang_monitor_start(const sentry_options_t *options)
{
    if (g_running || !options) {
        return 0;
    }

    g_timeout_ms = options->app_hang_timeout_ms;
    sentry__atomic_store(&g_stop, 0);
    sentry__cond_init(&g_wait_cond);
    if (sentry__thread_spawn(&g_thread, worker, NULL) != 0) {
        SENTRY_WARN("app-hang: failed to spawn watchdog thread");
        return 1;
    }

    g_running = true;
    sentry__app_hang_set_active(true);
    SENTRY_DEBUG("app-hang watchdog started");
    return 0;
}

void
sentry__app_hang_monitor_stop(void)
{
    if (!g_running) {
        return;
    }
    sentry__app_hang_set_active(false);
    sentry__atomic_store(&g_stop, 1);
    sentry__mutex_lock(&g_wait_mutex);
    sentry__cond_wake(&g_wait_cond);
    sentry__mutex_unlock(&g_wait_mutex);
    sentry__thread_join(g_thread);
    sentry__app_hang_latch_reset();
    g_running = false;
    // g_timeout_ms are intentionally NOT cleared here: the worker
    // (now joined) is their only reader, and start() always re-sets them, so
    // clearing would just introduce a data race.
    SENTRY_DEBUG("app-hang watchdog stopped");
}

#else // !SENTRY_HAS_THREAD_STACKWALK

// No thread stackwalker on this platform: a fired hang could only produce
// frameless events, so the watchdog is never started and stop is a no-op.
int
sentry__app_hang_monitor_start(const sentry_options_t *options)
{
    (void)options;
    SENTRY_DEBUG(
        "app-hang: no thread stackwalker for this platform, not starting");
    return 0;
}

void
sentry__app_hang_monitor_stop(void)
{
}

#endif // SENTRY_HAS_THREAD_STACKWALK
