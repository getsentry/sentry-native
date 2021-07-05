#include "sentry_core.h"
#include "sentry_testsupport.h"
#include <pthread.h>
#include <sentry.h>
#include <sentry_sync.h>

static sentry_mutex_t func_lock = SENTRY__MUTEX_INIT;

static void
send_envelope_test_concurrent(const sentry_envelope_t *envelope, void *data)
{
    sentry__mutex_lock(&func_lock);
    uint64_t *called = data;
    *called += 1;

    sentry_value_t event = sentry_envelope_get_event(envelope);
    if (sentry_value_is_null(event)) {
        sentry__mutex_unlock(&func_lock);
        return;
    }
    TEST_CHECK(!sentry_value_is_null(event));
    const char *event_id
        = sentry_value_as_string(sentry_value_get_by_key(event, "event_id"));
    TEST_CHECK_STRING_EQUAL(event_id, "4c035723-8638-4c3a-923f-2ab9d08b4018");

    if (*called == 1) {
        const char *msg = sentry_value_as_string(sentry_value_get_by_key(
            sentry_value_get_by_key(event, "message"), "formatted"));
        TEST_CHECK_STRING_EQUAL(msg, "Hello World!");
        const char *release
            = sentry_value_as_string(sentry_value_get_by_key(event, "release"));
        TEST_CHECK_STRING_EQUAL(release, "prod");
        const char *trans = sentry_value_as_string(
            sentry_value_get_by_key(event, "transaction"));
        TEST_CHECK_STRING_EQUAL(trans, "demo-trans");
    }
    sentry__mutex_unlock(&func_lock);
}

static void
init_framework(uint64_t *called)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_transport(options,
        sentry_new_function_transport(send_envelope_test_concurrent, called));
    sentry_options_set_release(options, "prod");
    sentry_options_set_require_user_consent(options, false);
    sentry_options_set_auto_session_tracking(options, true);
    sentry_options_set_debug(options, true);
    sentry_init(options);
}

SENTRY_TEST(multiple_inits)
{
    uint64_t called = 0;

    init_framework(&called);
    init_framework(&called);

    sentry_set_transaction("demo-trans");

    sentry_capture_event(sentry_value_new_message_event(
        SENTRY_LEVEL_INFO, "root", "Hello World!"));

    sentry_value_t obj = sentry_value_new_object();
    // something that is not a uuid, as this will be forcibly changed
    sentry_value_set_by_key(obj, "event_id", sentry_value_new_int32(1234));
    sentry_capture_event(obj);

    sentry_close();
    sentry_close();

    TEST_CHECK_INT_EQUAL(called, 4);
}

static void *
thread_worker(void *vargp)
{
    sentry_set_transaction("demo-trans");

    sentry_capture_event(sentry_value_new_message_event(
        SENTRY_LEVEL_INFO, "root", "Hello World!"));

    sentry_value_t obj = sentry_value_new_object();
    // something that is not a uuid, as this will be forcibly changed
    sentry_value_set_by_key(obj, "event_id", sentry_value_new_int32(1234));
    sentry_capture_event(obj);

    return NULL;
}

SENTRY_TEST(concurrent_init)
{
    uint64_t called_m = 0;
    sentry_threadid_t thread_id1, thread_id2, thread_id3;

    init_framework(&called_m);

    sentry__thread_init(&thread_id1);
    sentry__thread_init(&thread_id2);
    sentry__thread_init(&thread_id3);
    sentry__thread_spawn(&thread_id1, &thread_worker, NULL);
    sentry__thread_spawn(&thread_id2, &thread_worker, NULL);
    sentry__thread_spawn(&thread_id3, &thread_worker, NULL);
    sentry__thread_join(thread_id1);
    sentry__thread_join(thread_id2);
    sentry__thread_join(thread_id3);
    sentry__thread_free(&thread_id1);
    sentry__thread_free(&thread_id2);
    sentry__thread_free(&thread_id3);

    sentry_close();

    TEST_CHECK_INT_EQUAL(called_m, 7);
}