/* pthread_threadid_np() and CLOCK_UPTIME_RAW are Darwin extensions hidden when
 * a strict POSIX feature macro (e.g. _XOPEN_SOURCE, set transitively by
 * sentry_crash_context.h) is active. Re-expose them before any include. */
#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
#    define _DARWIN_C_SOURCE
#endif

#include "sentry_app_hang.h"

#include "sentry_options.h"

#if defined(SENTRY_APP_HANG_HOST_SUPPORTED)
#    include "sentry_sync.h"

#    if defined(SENTRY_PLATFORM_WINDOWS)
#        include <windows.h>
#    elif defined(SENTRY_PLATFORM_MACOS)
#        include <pthread.h>
#        include <stdatomic.h>
#        include <time.h>
#    endif
#endif

sentry_app_hang_decision_t
sentry__app_hang_decide(bool enabled, uint64_t hb, uint64_t now,
    uint64_t timeout_ms, uint64_t last_fired_hb)
{
    if (!enabled || hb == 0 || timeout_ms == 0) {
        /* A zero timeout would treat every poll as stale (now - hb is always
         * >= 0 once we pass the torn-read guard below), firing a fresh AppHang
         * on each heartbeat advance of a perfectly healthy app. Treat it as
         * "no detection" rather than a hang storm. */
        return SENTRY_APP_HANG_NO_ACTION;
    }
    if (now < hb) {
        /* Torn shmem read (possible on x86 for a non-atomic 64-bit load).
         * Treat as fresh — daemon will see the real value on the next tick. */
        return SENTRY_APP_HANG_NO_ACTION;
    }
    if ((now - hb) < timeout_ms) {
        return SENTRY_APP_HANG_NO_ACTION;
    }
    if (hb == last_fired_hb) {
        /* Already fired for this freeze. Stay quiet until the host heartbeats
         * again, which advances `hb` and re-arms detection. */
        return SENTRY_APP_HANG_NO_ACTION;
    }
    return SENTRY_APP_HANG_FIRE;
}

// Public setters
void
sentry_options_set_app_hang_enabled(sentry_options_t *opts, int enabled)
{
    if (opts) {
        opts->app_hang_enabled = !!enabled;
    }
}

void
sentry_options_set_app_hang_timeout_ms(
    sentry_options_t *opts, uint64_t timeout_ms)
{
    if (opts) {
        opts->app_hang_timeout_ms = timeout_ms;
    }
}

#if defined(SENTRY_APP_HANG_HOST_SUPPORTED)

/* Recursive (see SENTRY__MUTEX_INIT). Serializes the heartbeat body against
 * shutdown clearing the registration and unmapping the shmem behind it. */
static sentry_mutex_t g_app_hang_lock = SENTRY__MUTEX_INIT;
static sentry_crash_context_t *g_app_hang_shmem = NULL;

void
sentry__app_hang_lock(void)
{
    sentry__mutex_lock(&g_app_hang_lock);
}

void
sentry__app_hang_unlock(void)
{
    sentry__mutex_unlock(&g_app_hang_lock);
}

void
sentry__app_hang_set_shmem(sentry_crash_context_t *ctx)
{
    sentry__mutex_lock(&g_app_hang_lock);
    g_app_hang_shmem = ctx;
    sentry__mutex_unlock(&g_app_hang_lock);
}

#    if defined(SENTRY_PLATFORM_WINDOWS)

uint64_t
sentry__app_hang_now_ms(void)
{
    ULONGLONG ticks_100ns = 0;
    /* QueryUnbiasedInterruptTime is documented signal/SEH/wait-free; the
     * same source is read on both sides of the IPC. */
    if (!QueryUnbiasedInterruptTime(&ticks_100ns)) {
        return 0;
    }
    return (uint64_t)(ticks_100ns / 10000ULL);
}

static void
app_hang_record_heartbeat(sentry_crash_context_t *ctx)
{
    DWORD current_tid = GetCurrentThreadId();

    /* Self-register on the first heartbeat: CAS the current TID into the latch
     * slot iff still unset — the first thread to heartbeat wins and becomes the
     * monitored target. CAS (rather than a plain store) prevents a late call
     * from a different thread from silently overwriting a prior latch. */
    InterlockedCompareExchange64((LONG64 volatile *)&ctx->app_hang_target_tid,
        (LONG64)(uint64_t)current_tid, 0);

    /* Drop the heartbeat unless the latched thread is us, so a stray heartbeat
     * from another thread cannot mask a frozen monitored thread. The non-atomic
     * read can tear on x86; in that case the compare fails and we drop a
     * heartbeat, which the daemon absorbs. */
    uint64_t latched = ctx->app_hang_target_tid;
    if (latched == 0 || (DWORD)latched != current_tid) {
        return;
    }

    /* Relaxed 64-bit store. On x64 this is a single mov. On x86 the value
     * may tear, but that is OK — see the comment in sentry_crash_context.h. */
    ctx->app_hang_last_heartbeat_ms = sentry__app_hang_now_ms();
}

#    elif defined(SENTRY_PLATFORM_MACOS)

uint64_t
sentry__app_hang_now_ms(void)
{
    /* CLOCK_UPTIME_RAW is a monotonic clock that excludes time the system
     * was asleep. */
    struct timespec ts;
    if (clock_gettime(CLOCK_UPTIME_RAW, &ts) != 0) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}

static void
app_hang_record_heartbeat(sentry_crash_context_t *ctx)
{
    /* Obtain the portable 64-bit Mach thread id of the current thread; this
     * is the same value the daemon matches against via
     * thread_info(THREAD_IDENTIFIER_INFO). */
    uint64_t current_tid = 0;
    if (pthread_threadid_np(NULL, &current_tid) != 0 || current_tid == 0) {
        return;
    }

    /* Self-register on the first heartbeat: CAS the current TID into the latch
     * slot iff still unset — the first thread to heartbeat wins and becomes the
     * monitored target. The shmem field is declared `volatile uint64_t`; view
     * it as an atomic for the compare-exchange. */
    _Atomic uint64_t *slot
        = (_Atomic uint64_t *)(void *)&ctx->app_hang_target_tid;
    uint64_t expected = 0;
    atomic_compare_exchange_strong(slot, &expected, current_tid);

    /* Drop the heartbeat unless the latched thread is us, so a stray heartbeat
     * from another thread cannot mask a frozen monitored thread. */
    if (ctx->app_hang_target_tid != current_tid) {
        return;
    }

    /* Relaxed 64-bit store; aligned on a 64-bit target so it is atomic and
     * cannot tear. The daemon reads it with a relaxed load. */
    ctx->app_hang_last_heartbeat_ms = sentry__app_hang_now_ms();
}

#    endif

void
sentry_app_hang_heartbeat(void)
{
    /* Hold the lock across the whole body: it pins the shmem mapping so backend
     * shutdown cannot unmap it (in sentry__crash_ipc_free) while we dereference
     * `ctx`. The body is bounded and non-blocking, so shutdown waits at most
     * for one in-flight heartbeat. */
    sentry__app_hang_lock();
    sentry_crash_context_t *ctx = g_app_hang_shmem;
    if (ctx && ctx->app_hang_enabled) {
        app_hang_record_heartbeat(ctx);
    }
    sentry__app_hang_unlock();
}

#else /* host heartbeat not supported on this target */

void
sentry_app_hang_heartbeat(void)
{
    /* No-op on unsupported targets in this initial cut. */
}

#endif
