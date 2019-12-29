#include "../src/sentry_path.h"
#include "sentry_testsupport.h"
#include <sentry.h>

static void
init_consenting_sentry()
{
#ifdef __ANDROID__
#    define PREFIX "/data/local/tmp/"
#else
#    define PREFIX ""
#endif
    sentry_options_t *opts = sentry_options_new();
    sentry_options_set_database_path(opts, PREFIX ".test-db");
    sentry_options_set_dsn(opts, "http://foo@127.0.0.1/42");
    sentry_options_set_require_user_consent(opts, true);
    sentry_init(opts);
}

SENTRY_TEST(basic_consent_tracking)
{
    sentry_path_t *path = sentry__path_from_str(PREFIX ".test-db");
    sentry__path_remove_all(path);

    init_consenting_sentry();
    assert_int_equal(sentry_user_consent_get(), SENTRY_USER_CONSENT_UNKNOWN);
    sentry_shutdown();

    init_consenting_sentry();
    sentry_user_consent_give();
    assert_int_equal(sentry_user_consent_get(), SENTRY_USER_CONSENT_GIVEN);
    sentry_shutdown();
    init_consenting_sentry();
    assert_int_equal(sentry_user_consent_get(), SENTRY_USER_CONSENT_GIVEN);

    sentry_user_consent_revoke();
    assert_int_equal(sentry_user_consent_get(), SENTRY_USER_CONSENT_REVOKED);
    sentry_shutdown();
    init_consenting_sentry();
    assert_int_equal(sentry_user_consent_get(), SENTRY_USER_CONSENT_REVOKED);

    sentry_user_consent_reset();
    assert_int_equal(sentry_user_consent_get(), SENTRY_USER_CONSENT_UNKNOWN);
    sentry_shutdown();
    init_consenting_sentry();
    assert_int_equal(sentry_user_consent_get(), SENTRY_USER_CONSENT_UNKNOWN);
    sentry_shutdown();

    sentry__path_remove_all(path);
    sentry__path_free(path);
}
