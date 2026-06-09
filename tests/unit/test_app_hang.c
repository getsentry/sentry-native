#include "sentry_app_hang.h"
#include "sentry_testsupport.h"

#include <stdint.h>

SENTRY_TEST(app_hang_decide_disabled_returns_no_action)
{
    sentry_app_hang_decision_t d = sentry__app_hang_decide(
        /*enabled=*/false, /*hb=*/100, /*now=*/10000,
        /*timeout_ms=*/1000, /*last_fired_hb=*/0);
    TEST_CHECK_INT_EQUAL(d, SENTRY_APP_HANG_NO_ACTION);
}

SENTRY_TEST(app_hang_decide_no_heartbeat_yet_returns_no_action)
{
    sentry_app_hang_decision_t d = sentry__app_hang_decide(
        /*enabled=*/true, /*hb=*/0, /*now=*/10000,
        /*timeout_ms=*/1000, /*last_fired_hb=*/0);
    TEST_CHECK_INT_EQUAL(d, SENTRY_APP_HANG_NO_ACTION);
}

SENTRY_TEST(app_hang_decide_fresh_heartbeat_returns_no_action)
{
    sentry_app_hang_decision_t d = sentry__app_hang_decide(
        /*enabled=*/true, /*hb=*/9500, /*now=*/10000,
        /*timeout_ms=*/1000, /*last_fired_hb=*/0);
    TEST_CHECK_INT_EQUAL(d, SENTRY_APP_HANG_NO_ACTION);
}

SENTRY_TEST(app_hang_decide_stale_heartbeat_fires)
{
    sentry_app_hang_decision_t d = sentry__app_hang_decide(
        /*enabled=*/true, /*hb=*/5000, /*now=*/10000,
        /*timeout_ms=*/1000, /*last_fired_hb=*/0);
    TEST_CHECK_INT_EQUAL(d, SENTRY_APP_HANG_FIRE);
}

SENTRY_TEST(app_hang_decide_exact_timeout_boundary_fires)
{
    /* now - hb == timeout_ms is still stale (>= semantics) — fires. */
    sentry_app_hang_decision_t d = sentry__app_hang_decide(
        /*enabled=*/true, /*hb=*/9000, /*now=*/10000,
        /*timeout_ms=*/1000, /*last_fired_hb=*/0);
    TEST_CHECK_INT_EQUAL(d, SENTRY_APP_HANG_FIRE);
}

SENTRY_TEST(app_hang_decide_cooldown_holds_when_hb_unchanged)
{
    /* Already fired for hb=5000. A sustained freeze must NOT re-fire while the
     * heartbeat stays at the same value. */
    sentry_app_hang_decision_t d = sentry__app_hang_decide(
        /*enabled=*/true, /*hb=*/5000, /*now=*/20000,
        /*timeout_ms=*/1000, /*last_fired_hb=*/5000);
    TEST_CHECK_INT_EQUAL(d, SENTRY_APP_HANG_NO_ACTION);
}

SENTRY_TEST(app_hang_decide_re_arms_after_advance_then_stall)
{
    /* hb advanced past last_fired_hb → cooldown released; a fresh stall fires
     * again. */
    sentry_app_hang_decision_t d = sentry__app_hang_decide(
        /*enabled=*/true, /*hb=*/7000, /*now=*/12000,
        /*timeout_ms=*/1000, /*last_fired_hb=*/5000);
    TEST_CHECK_INT_EQUAL(d, SENTRY_APP_HANG_FIRE);
}

SENTRY_TEST(app_hang_decide_zero_timeout_returns_no_action)
{
    /* A zero timeout must not turn a healthy, heartbeating app into a stream of
     * spurious AppHang events — it is treated as "detection off". */
    sentry_app_hang_decision_t d = sentry__app_hang_decide(
        /*enabled=*/true, /*hb=*/9999, /*now=*/10000,
        /*timeout_ms=*/0, /*last_fired_hb=*/0);
    TEST_CHECK_INT_EQUAL(d, SENTRY_APP_HANG_NO_ACTION);
}

SENTRY_TEST(app_hang_decide_torn_read_now_less_than_hb_returns_no_action)
{
    /* On x86 a non-atomic 64-bit load can tear, producing now < hb. The
     * decision function treats this as fresh (no FIRE); the daemon sees the
     * real value on the next tick. */
    sentry_app_hang_decision_t d = sentry__app_hang_decide(
        /*enabled=*/true, /*hb=*/10000, /*now=*/5000,
        /*timeout_ms=*/1000, /*last_fired_hb=*/0);
    TEST_CHECK_INT_EQUAL(d, SENTRY_APP_HANG_NO_ACTION);
}
