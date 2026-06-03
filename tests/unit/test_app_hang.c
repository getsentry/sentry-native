#include "sentry_app_hang.h"
#include "sentry_testsupport.h"

#include <stdint.h>

SENTRY_TEST(app_hang_decide_disabled_returns_no_action)
{
    int new_count = 99;
    sentry_app_hang_decision_t d = sentry__app_hang_decide(
        /*enabled=*/false, /*hb=*/100, /*now=*/10000,
        /*timeout_ms=*/1000, /*last_fired_hb=*/0,
        /*consecutive_stale_ticks=*/0, &new_count);
    TEST_CHECK_INT_EQUAL(d, SENTRY_APP_HANG_NO_ACTION);
    /* Disabled path resets the counter. */
    TEST_CHECK_INT_EQUAL(new_count, 0);
}

SENTRY_TEST(app_hang_decide_no_heartbeat_yet_returns_no_action)
{
    int new_count = 99;
    sentry_app_hang_decision_t d = sentry__app_hang_decide(
        /*enabled=*/true, /*hb=*/0, /*now=*/10000,
        /*timeout_ms=*/1000, /*last_fired_hb=*/0,
        /*consecutive_stale_ticks=*/0, &new_count);
    TEST_CHECK_INT_EQUAL(d, SENTRY_APP_HANG_NO_ACTION);
    TEST_CHECK_INT_EQUAL(new_count, 0);
}

SENTRY_TEST(app_hang_decide_fresh_heartbeat_returns_no_action_and_resets)
{
    int new_count = 99;
    sentry_app_hang_decision_t d = sentry__app_hang_decide(
        /*enabled=*/true, /*hb=*/9500, /*now=*/10000,
        /*timeout_ms=*/1000, /*last_fired_hb=*/0,
        /*consecutive_stale_ticks=*/2, &new_count);
    TEST_CHECK_INT_EQUAL(d, SENTRY_APP_HANG_NO_ACTION);
    /* Fresh heartbeat resets the strike counter even mid-accumulation. */
    TEST_CHECK_INT_EQUAL(new_count, 0);
}

SENTRY_TEST(app_hang_decide_first_stale_tick_increments_does_not_fire)
{
    int new_count = -1;
    sentry_app_hang_decision_t d = sentry__app_hang_decide(
        /*enabled=*/true, /*hb=*/5000, /*now=*/10000,
        /*timeout_ms=*/1000, /*last_fired_hb=*/0,
        /*consecutive_stale_ticks=*/0, &new_count);
    TEST_CHECK_INT_EQUAL(d, SENTRY_APP_HANG_NO_ACTION);
    TEST_CHECK_INT_EQUAL(new_count, 1);
}

SENTRY_TEST(app_hang_decide_second_stale_tick_increments_does_not_fire)
{
    int new_count = -1;
    sentry_app_hang_decision_t d = sentry__app_hang_decide(
        /*enabled=*/true, /*hb=*/5000, /*now=*/10000,
        /*timeout_ms=*/1000, /*last_fired_hb=*/0,
        /*consecutive_stale_ticks=*/1, &new_count);
    TEST_CHECK_INT_EQUAL(d, SENTRY_APP_HANG_NO_ACTION);
    TEST_CHECK_INT_EQUAL(new_count, 2);
}

SENTRY_TEST(app_hang_decide_third_stale_tick_fires)
{
    int new_count = -1;
    sentry_app_hang_decision_t d = sentry__app_hang_decide(
        /*enabled=*/true, /*hb=*/5000, /*now=*/10000,
        /*timeout_ms=*/1000, /*last_fired_hb=*/0,
        /*consecutive_stale_ticks=*/2, &new_count);
    TEST_CHECK_INT_EQUAL(d, SENTRY_APP_HANG_FIRE);
    TEST_CHECK_INT_EQUAL(new_count, 3);
}

SENTRY_TEST(app_hang_decide_brief_hiccup_resets_strike_count)
{
    /* Simulate: 2 stale ticks, then a fresh heartbeat (counter resets),
     * then 1 stale tick → must NOT fire because we lost our accumulated
     * strikes when the heartbeat refreshed. */
    int after_hiccup = -1;
    sentry_app_hang_decision_t d = sentry__app_hang_decide(
        /*enabled=*/true, /*hb=*/9800, /*now=*/10000,
        /*timeout_ms=*/1000, /*last_fired_hb=*/0,
        /*consecutive_stale_ticks=*/2, &after_hiccup);
    TEST_CHECK_INT_EQUAL(d, SENTRY_APP_HANG_NO_ACTION);
    TEST_CHECK_INT_EQUAL(after_hiccup, 0);

    int after_one_stale = -1;
    d = sentry__app_hang_decide(/*enabled=*/true, /*hb=*/9800,
        /*now=*/11000, /*timeout_ms=*/1000, /*last_fired_hb=*/0,
        /*consecutive_stale_ticks=*/after_hiccup, &after_one_stale);
    TEST_CHECK_INT_EQUAL(d, SENTRY_APP_HANG_NO_ACTION);
    TEST_CHECK_INT_EQUAL(after_one_stale, 1);
}

SENTRY_TEST(app_hang_decide_cooldown_holds_when_hb_unchanged)
{
    /* Already fired for hb=5000. Subsequent ticks must NOT re-fire even
     * if 100 more stale ticks accumulate. Counter held at 0. */
    int new_count = -1;
    sentry_app_hang_decision_t d = sentry__app_hang_decide(
        /*enabled=*/true, /*hb=*/5000, /*now=*/20000,
        /*timeout_ms=*/1000, /*last_fired_hb=*/5000,
        /*consecutive_stale_ticks=*/0, &new_count);
    TEST_CHECK_INT_EQUAL(d, SENTRY_APP_HANG_NO_ACTION);
    TEST_CHECK_INT_EQUAL(new_count, 0);
}

SENTRY_TEST(app_hang_decide_re_arms_after_advance_then_stall)
{
    /* hb advanced past last_fired_hb → cooldown released; need 3 fresh
     * strikes again. */
    int after_strike1 = -1;
    sentry_app_hang_decision_t d = sentry__app_hang_decide(
        /*enabled=*/true, /*hb=*/7000, /*now=*/12000,
        /*timeout_ms=*/1000, /*last_fired_hb=*/5000,
        /*consecutive_stale_ticks=*/0, &after_strike1);
    TEST_CHECK_INT_EQUAL(d, SENTRY_APP_HANG_NO_ACTION);
    TEST_CHECK_INT_EQUAL(after_strike1, 1);

    int after_strike3 = -1;
    d = sentry__app_hang_decide(/*enabled=*/true, /*hb=*/7000,
        /*now=*/12000, /*timeout_ms=*/1000, /*last_fired_hb=*/5000,
        /*consecutive_stale_ticks=*/2, &after_strike3);
    TEST_CHECK_INT_EQUAL(d, SENTRY_APP_HANG_FIRE);
    TEST_CHECK_INT_EQUAL(after_strike3, 3);
}

SENTRY_TEST(app_hang_decide_exact_timeout_boundary_with_third_strike_fires)
{
    /* now - hb == timeout_ms is still stale (>= semantics) AND the third
     * strike has accumulated — fires. */
    int new_count = -1;
    sentry_app_hang_decision_t d = sentry__app_hang_decide(
        /*enabled=*/true, /*hb=*/9000, /*now=*/10000,
        /*timeout_ms=*/1000, /*last_fired_hb=*/0,
        /*consecutive_stale_ticks=*/2, &new_count);
    TEST_CHECK_INT_EQUAL(d, SENTRY_APP_HANG_FIRE);
    TEST_CHECK_INT_EQUAL(new_count, 3);
}

SENTRY_TEST(app_hang_decide_torn_read_now_less_than_hb_resets)
{
    /* On x86 a non-atomic 64-bit load can tear, producing now < hb. The
     * decision function treats this as fresh (no FIRE) and resets the
     * strike counter so the next non-torn observation starts clean. */
    int new_count = 99;
    sentry_app_hang_decision_t d = sentry__app_hang_decide(
        /*enabled=*/true, /*hb=*/10000, /*now=*/5000,
        /*timeout_ms=*/1000, /*last_fired_hb=*/0,
        /*consecutive_stale_ticks=*/2, &new_count);
    TEST_CHECK_INT_EQUAL(d, SENTRY_APP_HANG_NO_ACTION);
    TEST_CHECK_INT_EQUAL(new_count, 0);
}
