#include "sentry_alloc.h"
#include "sentry_backend.h"
#include "sentry_envelope.h"
#include "sentry_options.h"
#include "sentry_path.h"
#include "sentry_session.h"
#include "sentry_testsupport.h"
#include "sentry_value.h"

static void
send_envelope(sentry_envelope_t *envelope, void *data)
{
    uint64_t *called = data;
    *called += 1;

    TEST_CHECK_INT_EQUAL(sentry__envelope_get_item_count(envelope), 1);

    const sentry_envelope_item_t *item = sentry__envelope_get_item(envelope, 0);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry__envelope_item_get_header(item, "type")),
        "session");

    size_t buf_len;
    const char *buf = sentry__envelope_item_get_payload(item, &buf_len);
    sentry_value_t session = sentry_value_from_json(buf, buf_len);

    TEST_CHECK(sentry_value_is_true(sentry_value_get_by_key(session, "init")));
    TEST_CHECK_INT_EQUAL(
        sentry_value_get_type(sentry_value_get_by_key(session, "sid")),
        SENTRY_VALUE_TYPE_STRING);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(session, "status")),
        *called == 2 ? "crashed" : "exited");
    // did falls back to the installation ID since neither user sets an `id`
    TEST_CHECK(
        strlen(sentry_value_as_string(sentry_value_get_by_key(session, "did")))
        > 0);
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
        "test");

    sentry_value_decref(session);
    sentry_envelope_free(envelope);
}

SENTRY_TEST(session_basics)
{
    uint64_t called = 0;
    SENTRY_TEST_OPTIONS_NEW(options);
    // clear any leftover from previous test runs
    sentry__path_remove_all(options->database_path);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_transport_t *transport = sentry_transport_new(send_envelope);
    sentry_transport_set_state(transport, &called);
    sentry_options_set_transport(options, transport);
    sentry_options_set_release(options, "my_release");

    // the default environment is always `production` if not overwritten by the
    // OS environment variable `SENTRY_ENVIRONMENT`
    // (see https://develop.sentry.dev/sdk/event-payloads/#optional-attributes)
    TEST_CHECK_STRING_EQUAL(
        sentry_options_get_environment(options), "production");
    sentry_options_set_environment(options, "my_environment");
    TEST_CHECK_STRING_EQUAL(
        sentry_options_get_environment(options), "my_environment");
    char env[] = { 't', 'e', 's', 't' };
    sentry_options_set_environment_n(options, env, sizeof(env));
    sentry_init(options);

    // a session was already started by automatic session tracking
    sentry_value_t user = sentry_value_new_object();
    sentry_value_set_by_key(
        user, "email", sentry_value_new_string("foo@blabla.invalid"));
    sentry_set_user(user);

    sentry_end_session();
    sentry_start_session();

    user = sentry_value_new_object();
    sentry_value_set_by_key(
        user, "username", sentry_value_new_string("swatinem"));
    sentry_set_user(user);

    sentry_end_session_with_status(SENTRY_SESSION_STATUS_CRASHED);
    sentry_start_session();

    sentry_close();

    TEST_CHECK_INT_EQUAL(called, 3);
}

typedef struct {
    bool assert_session;
    uint64_t called;
} session_assertion_t;

static void
send_sampled_envelope(sentry_envelope_t *envelope, void *data)
{
    session_assertion_t *assertion = data;

    SENTRY_INFO("send_sampled_envelope");
    if (assertion->assert_session) {
        assertion->called += 1;

        SENTRY_INFO("assertion + 1");

        TEST_CHECK_INT_EQUAL(sentry__envelope_get_item_count(envelope), 1);

        const sentry_envelope_item_t *item
            = sentry__envelope_get_item(envelope, 0);
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(
                sentry__envelope_item_get_header(item, "type")),
            "session");

        size_t buf_len;
        const char *buf = sentry__envelope_item_get_payload(item, &buf_len);
        sentry_value_t session = sentry_value_from_json(buf, buf_len);

        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(session, "status")),
            "exited");
        TEST_CHECK_INT_EQUAL(
            sentry_value_as_int32(sentry_value_get_by_key(session, "errors")),
            100);

        sentry_value_decref(session);
    }
    sentry_envelope_free(envelope);
}

SENTRY_TEST(count_sampled_events)
{
    session_assertion_t assertion = { false, 0 };

    SENTRY_TEST_OPTIONS_NEW(options);
    // clear any leftover from previous test runs
    sentry__path_remove_all(options->database_path);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_transport_t *transport = sentry_transport_new(send_sampled_envelope);
    sentry_transport_set_state(transport, &assertion);
    sentry_options_set_transport(options, transport);
    sentry_options_set_release(options, "my_release");
    sentry_options_set_sample_rate(options, 0.5);
    sentry_init(options);

    for (int i = 0; i < 100; i++) {
        sentry_capture_event(
            sentry_value_new_message_event(SENTRY_LEVEL_ERROR, NULL, "foo"));
    }

    assertion.assert_session = true;
    sentry_close();

    TEST_CHECK_INT_EQUAL(assertion.called, 1);
}

typedef struct {
    bool enabled;
    uint64_t sessions;
    uint64_t events;
    const char *expected_release;
    const char *expected_environment;
} release_env_assertion_t;

static void
send_release_env_envelope(sentry_envelope_t *envelope, void *data)
{
    release_env_assertion_t *assertion = data;

    if (!assertion->enabled) {
        sentry_envelope_free(envelope);
        return;
    }

    sentry_value_t event = sentry_envelope_get_event(envelope);
    if (!sentry_value_is_null(event)) {
        assertion->events += 1;
        TEST_CHECK_STRING_EQUAL(
            sentry_value_as_string(sentry_value_get_by_key(event, "release")),
            assertion->expected_release);
        TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                    event, "environment")),
            assertion->expected_environment);
    }

    for (size_t i = 0; i < sentry__envelope_get_item_count(envelope); i++) {
        const sentry_envelope_item_t *item
            = sentry__envelope_get_item(envelope, i);
        const char *type = sentry_value_as_string(
            sentry__envelope_item_get_header(item, "type"));

        if (strcmp(type, "session") == 0) {
            assertion->sessions += 1;

            size_t buf_len;
            const char *buf = sentry__envelope_item_get_payload(item, &buf_len);
            sentry_value_t session = sentry_value_from_json(buf, buf_len);

            sentry_value_t attrs = sentry_value_get_by_key(session, "attrs");
            TEST_CHECK_STRING_EQUAL(
                sentry_value_as_string(
                    sentry_value_get_by_key(attrs, "release")),
                assertion->expected_release);
            TEST_CHECK_STRING_EQUAL(
                sentry_value_as_string(
                    sentry_value_get_by_key(attrs, "environment")),
                assertion->expected_environment);

            sentry_value_decref(session);
        }
    }
    sentry_envelope_free(envelope);
}

SENTRY_TEST(set_release_and_environment_late)
{
#if defined(SENTRY_PLATFORM_NX)
    SKIP_TEST();
#endif
    release_env_assertion_t assertion
        = { true, 0, 0, "late_release", "late_env" };

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry__path_remove_all(options->database_path);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_transport_t *transport
        = sentry_transport_new(send_release_env_envelope);
    sentry_transport_set_state(transport, &assertion);
    sentry_options_set_transport(options, transport);
    sentry_init(options);

    sentry_set_release("late_release");
    sentry_set_environment("late_env");

    sentry_capture_event(
        sentry_value_new_message_event(SENTRY_LEVEL_INFO, NULL, "test"));

    sentry_start_session();
    sentry_end_session();

    TEST_CHECK_INT_EQUAL(assertion.events, 1);
    TEST_CHECK_INT_EQUAL(assertion.sessions, 1);

    sentry_close();
}

SENTRY_TEST(update_release_and_environment_late)
{
    release_env_assertion_t assertion
        = { false, 0, 0, "updated_release", "updated_env" };

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry__path_remove_all(options->database_path);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_transport_t *transport
        = sentry_transport_new(send_release_env_envelope);
    sentry_transport_set_state(transport, &assertion);
    sentry_options_set_transport(options, transport);
    sentry_options_set_release(options, "initial");
    sentry_options_set_environment(options, "initial");
    sentry_init(options);

    sentry_end_session();

    sentry_set_release("updated_release");
    sentry_set_environment("updated_env");

    assertion.enabled = true;

    sentry_capture_event(
        sentry_value_new_message_event(SENTRY_LEVEL_INFO, NULL, "test"));

    sentry_start_session();
    sentry_end_session();

    TEST_CHECK_INT_EQUAL(assertion.events, 1);
    TEST_CHECK_INT_EQUAL(assertion.sessions, 1);

    sentry_close();
}

typedef struct {
    size_t calls;
    bool last_had_session;
    bool last_init;
    char last_did[64];
} session_hook_data_t;

static void
record_session_changed(sentry_backend_t *backend,
    const sentry_options_t *UNUSED(options), const sentry_session_t *session)
{
    session_hook_data_t *d = backend->data;
    d->calls++;
    d->last_had_session = !!session;
    d->last_init = session && session->init;
    d->last_did[0] = '\0';
    if (session && !sentry_value_is_null(session->distinct_id)) {
        snprintf(d->last_did, sizeof(d->last_did), "%s",
            sentry_value_as_string(session->distinct_id));
    }
}

static void
discard_envelope(sentry_envelope_t *envelope, void *UNUSED(data))
{
    sentry_envelope_free(envelope);
}

SENTRY_TEST(session_changed_backend_hook)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry__path_remove_all(options->database_path);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_transport(
        options, sentry_transport_new(discard_envelope));
    sentry_options_set_release(options, "my_release");

    session_hook_data_t data = { 0 };
    sentry_backend_t *backend = SENTRY_MAKE(sentry_backend_t);
    TEST_ASSERT(!!backend);
    backend->data = &data;
    backend->session_changed_func = record_session_changed;
    sentry_options_set_backend(options, backend);

    TEST_CHECK_INT_EQUAL(sentry_init(options), 0);
    // auto session tracking started a session during init
    TEST_CHECK_INT_EQUAL(data.calls, 1);
    TEST_CHECK(data.last_had_session);
    TEST_CHECK(data.last_init);

    // the first event carries the initial session update
    sentry_capture_event(
        sentry_value_new_message_event(SENTRY_LEVEL_INFO, NULL, "first event"));
    TEST_CHECK_INT_EQUAL(data.calls, 2);
    TEST_CHECK(data.last_had_session);
    TEST_CHECK(!data.last_init);

    // later events do not change the session
    sentry_capture_event(sentry_value_new_message_event(
        SENTRY_LEVEL_INFO, NULL, "second event"));
    TEST_CHECK_INT_EQUAL(data.calls, 3);

    sentry_set_user(sentry_value_new_user("user-id", NULL, NULL, NULL));
    TEST_CHECK_INT_EQUAL(data.calls, 4);
    TEST_CHECK_STRING_EQUAL(data.last_did, "user-id");

    sentry_end_session();
    TEST_CHECK_INT_EQUAL(data.calls, 5);
    TEST_CHECK(!data.last_had_session);

    // ending without a current session does not notify
    sentry_end_session();
    TEST_CHECK_INT_EQUAL(data.calls, 5);

    sentry_start_session();
    TEST_CHECK_INT_EQUAL(data.calls, 6);
    TEST_CHECK(data.last_had_session);
    TEST_CHECK(data.last_init);

    sentry_close();
    TEST_CHECK_INT_EQUAL(data.calls, 7);
    TEST_CHECK(!data.last_had_session);
}
