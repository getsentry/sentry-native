#include "sentry_app_hang.h"

#include "sentry_options.h"

#if defined(SENTRY_PLATFORM_WINDOWS) && !defined(SENTRY_PLATFORM_XBOX)            \
    && defined(SENTRY_BACKEND_NATIVE)
#    include <windows.h>
#endif

sentry_app_hang_decision_t
sentry__app_hang_decide(bool enabled, uint64_t hb, uint64_t now,
    uint64_t timeout_ms, uint64_t last_fired_hb,
    int consecutive_stale_ticks, int *out_consecutive_stale_ticks)
{
    /* Fresh or disabled paths reset the counter. */
    if (!enabled || hb == 0) {
        *out_consecutive_stale_ticks = 0;
        return SENTRY_APP_HANG_NO_ACTION;
    }
    if (now < hb) {
        /* Torn shmem read (possible on x86 for a non-atomic 64-bit load).
         * Treat as fresh — daemon will see the real value on the next tick. */
        *out_consecutive_stale_ticks = 0;
        return SENTRY_APP_HANG_NO_ACTION;
    }
    if ((now - hb) < timeout_ms) {
        *out_consecutive_stale_ticks = 0;
        return SENTRY_APP_HANG_NO_ACTION;
    }
    if (hb == last_fired_hb) {
        /* Already fired for this freeze. Stay quiet and hold the counter at
         * zero so we re-arm cleanly once the host heartbeats again. */
        *out_consecutive_stale_ticks = 0;
        return SENTRY_APP_HANG_NO_ACTION;
    }
    /* Stale and not in cooldown — accumulate a strike. */
    int new_count = consecutive_stale_ticks + 1;
    *out_consecutive_stale_ticks = new_count;
    if (new_count >= SENTRY_APP_HANG_STRIKES_REQUIRED) {
        return SENTRY_APP_HANG_FIRE;
    }
    return SENTRY_APP_HANG_NO_ACTION;
}

/* Public setters (always compiled, no platform guard — they only mutate the
 * options struct). */
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

#if defined(SENTRY_PLATFORM_WINDOWS) && !defined(SENTRY_PLATFORM_XBOX)            \
    && defined(SENTRY_BACKEND_NATIVE)

static sentry_crash_context_t *volatile g_app_hang_shmem = NULL;

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

void
sentry__app_hang_set_shmem(sentry_crash_context_t *ctx)
{
    g_app_hang_shmem = ctx;
}

void
sentry_app_hang_set_target_thread(void)
{
    sentry_crash_context_t *ctx = g_app_hang_shmem;
    if (!ctx || !ctx->app_hang_enabled) {
        return;
    }

    /* CAS the current TID into the latch slot iff still unset. If another
     * thread races and wins, our call is silently dropped — the API contract
     * is "first caller wins, idempotent for that caller". CAS (rather than a
     * plain store) prevents a late call from a different thread from
     * silently overwriting a prior latch. */
    DWORD current_tid = GetCurrentThreadId();
    InterlockedCompareExchange64((LONG64 volatile *)&ctx->app_hang_target_tid,
        (LONG64)(uint64_t)current_tid, 0);
}

void
sentry_app_hang_heartbeat(void)
{
    sentry_crash_context_t *ctx = g_app_hang_shmem;
    if (!ctx || !ctx->app_hang_enabled) {
        return;
    }

    /* Refresh-only: requires a prior sentry_app_hang_set_target_thread()
     * call from this thread. Drops the heartbeat if no target is latched,
     * or if the latched thread is not us. The non-atomic read can tear on
     * x86; in that case the compare fails and we drop a heartbeat, which
     * the daemon's strike counter absorbs. */
    DWORD current_tid = GetCurrentThreadId();
    uint64_t latched = ctx->app_hang_target_tid;
    if (latched == 0 || (DWORD)latched != current_tid) {
        return;
    }

    /* Relaxed 64-bit store. On x64 this is a single mov. On x86 the value
     * may tear, but that is OK — see the comment in sentry_crash_context.h. */
    ctx->app_hang_last_heartbeat_ms = sentry__app_hang_now_ms();
}

#else /* non-Windows or Xbox */

void
sentry_app_hang_set_target_thread(void)
{
    /* No-op on non-Windows targets in this initial cut. */
}

void
sentry_app_hang_heartbeat(void)
{
    /* No-op on non-Windows targets in this initial cut. */
}

#endif
