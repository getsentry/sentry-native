#include <sentry.h>
#include <path.hpp>
#include <vendor/catch.hpp>

static void init_consenting_sentry() {
    sentry_options_t *opts = sentry_options_new();
    sentry_options_set_database_path(opts, ".test-db");
    sentry_options_set_dsn(opts, "http://foo@127.0.0.1/42");
    sentry_options_set_require_user_consent(opts, true);
    sentry_init(opts);
}

TEST_CASE("basic consent tracking", "[api]") {
    sentry::Path(".test-db").remove_all();

    init_consenting_sentry();
    REQUIRE(sentry_user_consent_get() == SENTRY_USER_CONSENT_UNKNOWN);
    sentry_shutdown();

    init_consenting_sentry();
    sentry_user_consent_give();
    REQUIRE(sentry_user_consent_get() == SENTRY_USER_CONSENT_GIVEN);
    sentry_shutdown();
    init_consenting_sentry();
    REQUIRE(sentry_user_consent_get() == SENTRY_USER_CONSENT_GIVEN);

    sentry_user_consent_revoke();
    REQUIRE(sentry_user_consent_get() == SENTRY_USER_CONSENT_REVOKED);
    sentry_shutdown();
    init_consenting_sentry();
    REQUIRE(sentry_user_consent_get() == SENTRY_USER_CONSENT_REVOKED);

    sentry_user_consent_reset();
    REQUIRE(sentry_user_consent_get() == SENTRY_USER_CONSENT_UNKNOWN);
    sentry_shutdown();
    init_consenting_sentry();
    REQUIRE(sentry_user_consent_get() == SENTRY_USER_CONSENT_UNKNOWN);
    sentry_shutdown();

    sentry::Path(".test-db").remove_all();
}
