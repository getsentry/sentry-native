#include "sentry_testsupport.h"
#include "sentry_transport.h"
#include "sentry_utils.h"
#include <sentry.h>

SENTRY_TEST(rate_limit_parsing)
{
    uint64_t now = sentry__msec_time();
    sentry_rate_limiter_t *rl = sentry__rate_limiter_new();
    TEST_CHECK(sentry__rate_limiter_update_from_header(
        rl, "120:error:project, 60:session:foo, +30::bar"));

    TEST_CHECK(
        sentry__rate_limiter_get_disabled_until(rl, SENTRY_RL_CATEGORY_ERROR)
        >= now + 120000);
    TEST_CHECK(sentry__rate_limiter_get_disabled_until(
                   rl, SENTRY_RL_CATEGORY_TRANSACTION)
        == 0);
    TEST_CHECK(
        sentry__rate_limiter_get_disabled_until(rl, SENTRY_RL_CATEGORY_SESSION)
        >= now + 60000);
    TEST_CHECK(
        sentry__rate_limiter_get_disabled_until(rl, SENTRY_RL_CATEGORY_ANY)
        >= now + 30000);
    TEST_CHECK(
        sentry__rate_limiter_get_disabled_until(rl, SENTRY_RL_CATEGORY_ANY)
        <= now + 60000);

    sentry__rate_limiter_update_from_http_retry_after(rl, "60");
    TEST_CHECK(
        sentry__rate_limiter_get_disabled_until(rl, SENTRY_RL_CATEGORY_ANY)
        >= now + 60000);

    sentry__rate_limiter_free(rl);
}
