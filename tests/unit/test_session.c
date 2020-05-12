#include "sentry_envelope.h"
#include "sentry_session.h"
#include "sentry_testsupport.h"
#include "sentry_value.h"
#include <sentry.h>

static void
send_envelope(sentry_envelope_t *envelope, void *state)
{
    uint64_t *called = state;
    *called += 1;

    TEST_CHECK_INT_EQUAL(sentry__envelope_get_item_count(envelope), 1);

    const sentry_envelope_item_t *item = sentry__envelope_get_item(envelope, 0);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry__envelope_item_get_header(item, "type")),
        "session");

    size_t buf_len;
    const char *buf = sentry__envelope_item_get_payload(item, &buf_len);
    sentry_value_t session = sentry__value_from_json(buf, buf_len);

    TEST_CHECK(sentry_value_is_true(sentry_value_get_by_key(session, "init")));
    TEST_CHECK_INT_EQUAL(
        sentry_value_get_type(sentry_value_get_by_key(session, "sid")),
        SENTRY_VALUE_TYPE_STRING);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(session, "status")),
        "exited");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(session, "did")),
        "foo@blabla.invalid");
    TEST_CHECK_INT_EQUAL(
        sentry_value_as_int32(sentry_value_get_by_key(session, "errors")), 0);
    TEST_CHECK_INT_EQUAL(
        sentry_value_get_type(sentry_value_get_by_key(session, "started")),
        SENTRY_VALUE_TYPE_STRING);

    sentry_value_type_t duration_type
        = sentry_value_get_type(sentry_value_get_by_key(session, "duration"));
    TEST_CHECK(duration_type == SENTRY_VALUE_TYPE_DOUBLE
        || duration_type == SENTRY_VALUE_TYPE_INT32);

    sentry_value_t attrs = sentry_value_get_by_key(session, "attrs");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(attrs, "release")),
        "my_release");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(attrs, "environment")),
        "my_environment");

    sentry_value_decref(session);
    sentry_envelope_free(envelope);
}

SENTRY_TEST(session_basics)
{
    uint64_t called = 0;
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_transport(
        options, sentry_transport_new(send_envelope, &called));
    sentry_options_set_release(options, "my_release");
    sentry_options_set_environment(options, "my_environment");
    sentry_init(options);

    sentry_start_session();

    sentry_value_t user = sentry_value_new_object();
    sentry_value_set_by_key(
        user, "email", sentry_value_new_string("foo@blabla.invalid"));
    sentry_set_user(user);

    sentry_shutdown();

    TEST_CHECK_INT_EQUAL(called, 1);
}
