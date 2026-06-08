#ifndef SENTRY_APP_HANG_H_INCLUDED
#define SENTRY_APP_HANG_H_INCLUDED

#include "sentry_boot.h"

#include <stdbool.h>
#include <stdint.h>

/* The host-side heartbeat machinery (clock, latch, shmem registration) is
 * available on the native backend on macOS. Windows, Linux, and other targets
 * fall back to no-op stubs. */
#if defined(SENTRY_PLATFORM_MACOS) && defined(SENTRY_BACKEND_NATIVE)
#    define SENTRY_APP_HANG_HOST_SUPPORTED 1
#endif

#if defined(SENTRY_APP_HANG_HOST_SUPPORTED)
#    include "sentry_crash_context.h"
#endif

/**
 * Decision returned by the pure decision function. Kept tiny so it can be
 * exercised in unit tests without involving the daemon or shared memory.
 */
typedef enum {
    SENTRY_APP_HANG_NO_ACTION = 0,
    SENTRY_APP_HANG_FIRE = 1,
} sentry_app_hang_decision_t;

/* Number of consecutive timer ticks the daemon must observe a stale
 * heartbeat before firing. Smooths over brief hiccups (GC pauses, swap, OS
 * scheduler quanta) at the cost of ~SENTRY_APP_HANG_STRIKES_REQUIRED-1
 * extra poll periods of detection latency. */
#define SENTRY_APP_HANG_STRIKES_REQUIRED 3

/**
 * Pure function: should we fire an app-hang event right now?
 *
 *  - `enabled`:                  the host has app-hang detection turned on.
 *  - `hb`:                       last heartbeat timestamp (host clock; 0 means
 *                                "never heartbeated yet").
 *  - `now`:                      daemon's current observation of the same
 * clock.
 *  - `timeout_ms`:               staleness threshold.
 *  - `last_fired_hb`:            the `hb` value the daemon last fired for; used
 *                                as cooldown so a sustained freeze fires once.
 *  - `consecutive_stale_ticks`:  caller-tracked count of consecutive ticks on
 *                                which the heartbeat was observed stale.
 *  - `out_consecutive_stale_ticks` (out): updated counter the caller should
 *                                store. 0 if reset, otherwise incremented.
 *
 * Returns SENTRY_APP_HANG_FIRE iff: enabled, hb != 0, (now - hb) >= timeout_ms,
 * hb != last_fired_hb, AND the updated stale-tick counter reaches
 * SENTRY_APP_HANG_STRIKES_REQUIRED.
 */
sentry_app_hang_decision_t sentry__app_hang_decide(bool enabled, uint64_t hb,
    uint64_t now, uint64_t timeout_ms, uint64_t last_fired_hb,
    int consecutive_stale_ticks, int *out_consecutive_stale_ticks);

#if defined(SENTRY_APP_HANG_HOST_SUPPORTED)
/**
 * Called from the native backend startup path. Stores `ctx` so that
 * subsequent `sentry_app_hang_heartbeat()` calls have somewhere to write.
 * Passing NULL clears the registration on backend shutdown.
 *
 * The pointer is stored in a `volatile` global; ordering with shmem field
 * initialization is the caller's responsibility (the backend writes options
 * into shmem before calling this).
 */
void sentry__app_hang_set_shmem(sentry_crash_context_t *ctx);

/**
 * Return a millisecond-resolution unbiased timestamp shared between host and
 * daemon. Exposed for the daemon to call as well.
 */
uint64_t sentry__app_hang_now_ms(void);
#endif

#endif
