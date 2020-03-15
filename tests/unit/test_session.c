#include "sentry_testsupport.h"
#include "sentry_session.h"
#include <sentry.h>

static void
send_envelope(sentry_envelope_t *envelope, void *data)
{
    uint64_t *called = data;
    *called += 1;

    size_t buf_len;
    char *buf = sentry_envelope_serialize(envelope, &buf_len);
    sentry_free(buf);
}

SENTRY_TEST(session_basics)
{
    uint64_t called = 0;
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_transport(
        options, sentry_new_function_transport(send_envelope, &called));
    sentry_options_set_release(options, "my_release");
    sentry_init(options);

    sentry_start_session();

    sentry_value_t user = sentry_value_new_object();
    sentry_value_set_by_key(
        user, "email", sentry_value_new_string("foo@blabla.invalid"));
    sentry_set_user(user);

    sentry_shutdown();

    TEST_CHECK_INT_EQUAL(called, 1);
}
