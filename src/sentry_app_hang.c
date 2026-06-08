/* pthread_threadid_np() and CLOCK_UPTIME_RAW are Darwin extensions hidden when
 * a strict POSIX feature macro (e.g. _XOPEN_SOURCE, set transitively by
 * sentry_crash_context.h) is active. Re-expose them before any include. */
#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
#    define _DARWIN_C_SOURCE
#endif

#include "sentry_app_hang.h"

#include "sentry_options.h"

#if defined(SENTRY_APP_HANG_HOST_SUPPORTED)
#    include <pthread.h>
#    include <stdatomic.h>
#    include <time.h>
#endif

sentry_app_hang_decision_t
sentry__app_hang_decide(bool enabled, uint64_t hb, uint64_t now,
    uint64_t timeout_ms, uint64_t last_fired_hb)
{
    if (!enabled || hb == 0) {
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

static sentry_crash_context_t *volatile g_app_hang_shmem = NULL;

void
sentry__app_hang_set_shmem(sentry_crash_context_t *ctx)
{
    g_app_hang_shmem = ctx;
}

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

void
sentry_app_hang_heartbeat(void)
{
    sentry_crash_context_t *ctx = g_app_hang_shmem;
    if (!ctx || !ctx->app_hang_enabled) {
        return;
    }

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

#else /* host heartbeat not supported on this target */

void
sentry_app_hang_heartbeat(void)
{
    /* No-op on non-macOS targets in this initial cut. */
}

#endif
