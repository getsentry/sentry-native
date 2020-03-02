#include "sentry_testsupport.h"
#include "transports/sentry_function_transport.h"
#include <sentry.h>

static void
send_envelope(sentry_envelope_t *envelope, void *data)
{
    uint64_t *called = data;
    *called += 1;

    sentry_value_t event = sentry_envelope_get_event(envelope);
    TEST_CHECK(!sentry_value_is_null(event));
    const char *event_id
        = sentry_value_as_string(sentry_value_get_by_key(event, "event_id"));
    TEST_CHECK_STRING_EQUAL(event_id, "4c035723-8638-4c3a-923f-2ab9d08b4018");
    const char *msg = sentry_value_as_string(sentry_value_get_by_key(
        sentry_value_get_by_key(event, "message"), "formatted"));
    TEST_CHECK_STRING_EQUAL(msg, "Hello World!");
    const char *release
        = sentry_value_as_string(sentry_value_get_by_key(event, "release"));
    TEST_CHECK_STRING_EQUAL(release, "prod");
    const char *trans
        = sentry_value_as_string(sentry_value_get_by_key(event, "transaction"));
    TEST_CHECK_STRING_EQUAL(trans, "demo-trans");
}

SENTRY_TEST(basic_function_transport)
{
    uint64_t called = 0;

    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_transport(
        options, sentry_new_function_transport(send_envelope, &called));
    sentry_options_set_release(options, "prod");
    sentry_init(options);

    sentry_set_transaction("demo-trans");

    sentry_capture_event(sentry_value_new_message_event(
        SENTRY_LEVEL_INFO, "root", "Hello World!"));

    sentry_shutdown();

    TEST_CHECK_INT_EQUAL(called, 1);
}
