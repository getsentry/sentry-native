// In-process app-hang detection, shared state and helpers. The lock-free latch,
// heartbeat API, capture predicate, and monotonic clock are the app-thread hot
// path: app threads write the latch via the heartbeat, the watchdog worker in
// sentry_app_hang_monitor.c reads it. Event assembly
// (sentry__app_hang_make_event) lives here too but runs on the watchdog worker,
// not on app threads.
#include "sentry_app_hang_latch.h"
#include "sentry_sync.h"

#include <stdint.h>
#include <stdio.h>

#if defined(SENTRY_PLATFORM_WINDOWS)
#    include <windows.h>
#elif defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
#    include <sys/syscall.h>
#    include <time.h>
#    include <unistd.h>
#else // SENTRY_PLATFORM_MACOS and other POSIX
#    include <pthread.h>
#    include <time.h>
#endif

bool
sentry__app_hang_should_capture(
    uint64_t hb, uint64_t now, uint64_t timeout_ms, uint64_t last_fired_hb)
{
    if (hb == 0 || timeout_ms == 0) {
        return false;
    }
    if (now < hb || (now - hb) < timeout_ms) {
        return false;
    }
    if (hb == last_fired_hb) {
        return false; // already fired for this freeze
    }
    return true;
}

uint64_t
sentry__app_hang_now_ms(void)
{
#if defined(SENTRY_PLATFORM_WINDOWS)
    return (uint64_t)GetTickCount64();
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
#endif
}

// The latch is touched by app threads (writers, via the heartbeat) and the
// single watchdog worker (reader). Rather than a mutex -- which would put a
// lock on the heartbeat hot path, the very thing we're trying to detect
// stalling -- the two fields are accessed with 64-bit atomics:
//
//   - last_heartbeat_ms: written on every heartbeat, read by the worker.
//     Needs 64-bit-atomic access so a 32-bit platform can't observe a torn
//     half-updated timestamp.
//   - target_tid: write-once (0 -> first heartbeating tid). The worker only
//     uses it as the sampler argument, and the lone transition is benign, so a
//     relaxed read is sufficient; we use the same atomic helpers for clarity.
//
// Both fields use the sentry__atomic_*_u64 helpers, which provide full 64-bit
// atomic access even on 32-bit platforms
static uint64_t g_target_tid = 0;
static uint64_t g_last_heartbeat_ms = 0;
static volatile long g_app_hang_active = 0;

void
sentry__app_hang_set_active(bool active)
{
    sentry__atomic_store(&g_app_hang_active, active ? 1 : 0);
}

uint64_t
sentry__app_hang_current_tid(void)
{
#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_ANDROID)
    return (uint64_t)syscall(SYS_gettid);
#elif defined(SENTRY_PLATFORM_MACOS)
    uint64_t tid = 0;
    pthread_threadid_np(pthread_self(), &tid);
    return tid;
#elif defined(SENTRY_PLATFORM_WINDOWS)
    return (uint64_t)GetCurrentThreadId();
#else
    return 0;
#endif
}

void
sentry__app_hang_latch_read(sentry_app_hang_latch_t *out)
{
    out->target_tid = sentry__atomic_fetch_u64(&g_target_tid);
    out->last_heartbeat_ms = sentry__atomic_fetch_u64(&g_last_heartbeat_ms);
}

void
sentry__app_hang_latch_reset(void)
{
    sentry__atomic_store_u64(&g_target_tid, 0);
    sentry__atomic_store_u64(&g_last_heartbeat_ms, 0);
}

void
sentry_app_hang_heartbeat(void)
{
    if (!sentry__atomic_fetch(&g_app_hang_active)) {
        return;
    }
    uint64_t tid = sentry__app_hang_current_tid();

    uint64_t target = sentry__atomic_fetch_u64(&g_target_tid);
    if (target == 0) {
        // Latch the first heartbeating thread.
        sentry__atomic_store_u64(&g_target_tid, tid);
        target = tid;
    }
    if (target == tid) {
        // ignore heartbeats from other threads
        sentry__atomic_store_u64(
            &g_last_heartbeat_ms, sentry__app_hang_now_ms());
    }
}

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
