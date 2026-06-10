#ifndef SENTRY_APP_HANG_H_INCLUDED
#define SENTRY_APP_HANG_H_INCLUDED

#include "sentry_boot.h"

#include <stdbool.h>
#include <stdint.h>

/* The host-side heartbeat machinery (clock, latch, shmem registration) is
 * available on the native backend on Windows (non-Xbox), macOS, and Linux.
 * Android and other targets fall back to no-op stubs. */
#if (((defined(SENTRY_PLATFORM_WINDOWS) && !defined(SENTRY_PLATFORM_XBOX))        \
         || defined(SENTRY_PLATFORM_MACOS) || defined(SENTRY_PLATFORM_LINUX)))    \
    && defined(SENTRY_BACKEND_NATIVE)
#    define SENTRY_APP_HANG_HOST_SUPPORTED 1
#endif

#if defined(SENTRY_APP_HANG_HOST_SUPPORTED)
#    include "sentry_crash_context.h"
#endif

/**
 * Decision returned by the pure decision function.
 */
typedef enum {
    SENTRY_APP_HANG_NO_ACTION = 0,
    SENTRY_APP_HANG_FIRE = 1,
} sentry_app_hang_decision_t;

/**
 * Pure function: should we fire an app-hang event right now?
 *
 *  - `enabled`:        the host has app-hang detection turned on.
 *  - `hb`:             last heartbeat timestamp (host clock; 0 means
 *                      "never heartbeated yet").
 *  - `now`:            daemon's current observation of the same clock.
 *  - `timeout_ms`:     staleness threshold.
 *  - `last_fired_hb`:  the `hb` value the daemon last fired for; used as
 *                      cooldown so a sustained freeze fires once.
 *
 * Returns SENTRY_APP_HANG_FIRE if: enabled, hb != 0, (now - hb) >= timeout_ms,
 * and hb != last_fired_hb.
 */
sentry_app_hang_decision_t sentry__app_hang_decide(bool enabled, uint64_t hb,
    uint64_t now, uint64_t timeout_ms, uint64_t last_fired_hb);

#if defined(SENTRY_APP_HANG_HOST_SUPPORTED)
/**
 * Called from the native backend startup path. Stores `ctx` so that
 * subsequent `sentry_app_hang_heartbeat()` calls have somewhere to write.
 * Passing NULL clears the registration on backend shutdown.
 *
 * Access to the stored pointer is serialized by the app-hang lock (see
 * `sentry__app_hang_lock`); ordering with shmem field initialization is the
 * caller's responsibility (the backend writes options into shmem before
 * calling this).
 */
void sentry__app_hang_set_shmem(sentry_crash_context_t *ctx);

/**
 * Serialize heartbeat access against teardown. The backend must hold this lock
 * across BOTH clearing the shmem registration (`sentry__app_hang_set_shmem(
 * NULL)`) AND freeing the underlying mapping, so that an in-flight
 * `sentry_app_hang_heartbeat()` on another thread cannot write to memory that
 * is about to be unmapped. The lock is recursive.
 */
void sentry__app_hang_lock(void);
void sentry__app_hang_unlock(void);

/**
 * Return a millisecond-resolution unbiased timestamp shared between host and
 * daemon. Exposed for the daemon to call as well.
 */
uint64_t sentry__app_hang_now_ms(void);
#endif

#endif
