#include "sentry_core.h"
#include "sentry_database.h"
#include "sentry_envelope.h"
#include "sentry_options.h"
#include "sentry_path.h"
#include "sentry_testsupport.h"
#include "sentry_transport.h"

#include <sentry_sync.h>

#ifdef SENTRY__MUTEX_INIT_DYN
SENTRY__MUTEX_INIT_DYN(g_test_check_mutex)
#else
static sentry_mutex_t g_test_check_mutex = SENTRY__MUTEX_INIT;
#endif

static void
send_envelope_test_concurrent(sentry_envelope_t *envelope, void *data)
{
    SENTRY__MUTEX_INIT_DYN_ONCE(g_test_check_mutex);

    sentry__atomic_fetch_and_add((long *)data, 1);

    sentry_value_t event = sentry_envelope_get_event(envelope);
    if (!sentry_value_is_null(event)) {
        const char *event_id = sentry_value_as_string(
            sentry_value_get_by_key(event, "event_id"));
        // Protect the test check with a mutex since the test framework
        // global variables that track checks are not thread-safe.
        sentry__mutex_lock(&g_test_check_mutex);
        TEST_CHECK_STRING_EQUAL(
            event_id, "4c035723-8638-4c3a-923f-2ab9d08b4018");
        sentry__mutex_unlock(&g_test_check_mutex);
    }
    sentry_envelope_free(envelope);
}

static void
init_framework(long *called)
{
    SENTRY__MUTEX_INIT_DYN_ONCE(g_test_check_mutex);

    sentry__mutex_lock(&g_test_check_mutex);
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry__mutex_unlock(&g_test_check_mutex);

    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");

    sentry_transport_t *transport
        = sentry_transport_new(send_envelope_test_concurrent);
    sentry_transport_set_state(transport, called);
    sentry_options_set_transport(options, transport);

    sentry_options_set_release(options, "prod");
    sentry_options_set_require_user_consent(options, false);
    sentry_options_set_auto_session_tracking(options, true);

    sentry_init(options);
}

SENTRY_TEST(multiple_inits)
{
    long called = 0;

    init_framework(&called);
    init_framework(&called);

    sentry_set_transaction("demo-trans");

    sentry_capture_event(sentry_value_new_message_event(
        SENTRY_LEVEL_INFO, "root", "Hello World!"));

    sentry_value_t obj = sentry_value_new_object();
    // something that is not a UUID, as this will be forcibly changed
    sentry_value_set_by_key(obj, "event_id", sentry_value_new_int32(1234));
    sentry_capture_event(obj);

    sentry_close();
    sentry_close();

    TEST_CHECK_INT_EQUAL(called, 4);
}

typedef struct {
    sentry_envelope_t *queued;
    long send_started;
    long release_send;
    long send_count;
    long dump_count;
} suspend_transport_state_t;

static void
suspend_send(sentry_envelope_t *envelope, void *data)
{
    suspend_transport_state_t *state = data;
    sentry__atomic_store(&state->send_started, 1);
    while (!sentry__atomic_fetch(&state->release_send)) {
        sentry__thread_yield();
    }
    state->queued = envelope;
    sentry__atomic_fetch_and_add(&state->send_count, 1);
}

static size_t
suspend_dump(sentry_run_t *run, void *data)
{
    (void)run;
    suspend_transport_state_t *state = data;
    sentry__atomic_fetch_and_add(&state->dump_count, 1);
    if (!state->queued) {
        return 0;
    }
    sentry_envelope_free(state->queued);
    state->queued = NULL;
    return 1;
}

SENTRY_THREAD_FN
suspend_send_thread(void *data)
{
    sentry__transport_send_envelope(data, sentry__envelope_new());
    return 0;
}

SENTRY_TEST(transport_suspend)
{
    sentry_path_t *database_path = sentry__path_from_str(
        SENTRY_TEST_PATH_PREFIX ".transport-crash-mode");
    TEST_ASSERT(!!database_path);
    sentry__path_remove_all(database_path);
    TEST_ASSERT(!sentry__path_create_dir_all(database_path));
    sentry_run_t *run = sentry__run_new(database_path);
    TEST_ASSERT(!!run);

    suspend_transport_state_t state = { 0 };
    sentry_transport_t *transport = sentry_transport_new(suspend_send);
    TEST_ASSERT(!!transport);
    sentry_transport_set_state(transport, &state);
    sentry__transport_set_dump_func(transport, suspend_dump);
    sentry_options_t startup_options = { 0 };
    startup_options.run = run;
    TEST_CHECK(!sentry__transport_startup(transport, &startup_options));

    sentry_threadid_t thread;
    sentry__thread_init(&thread);
    TEST_ASSERT(!sentry__thread_spawn(&thread, suspend_send_thread, transport));
    while (!sentry__atomic_fetch(&state.send_started)) {
        sentry__thread_yield();
    }

    sentry__transport_suspend(transport);
    sentry__atomic_store(&state.release_send, 1);
    sentry__thread_join(thread);

    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&state.send_count), 1);
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&state.dump_count), 1);
    TEST_CHECK(!state.queued);

    sentry__transport_send_envelope(transport, sentry__envelope_new());
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch(&state.send_count), 1);
    TEST_CHECK(sentry__atomic_fetch(&run->retain));

    sentry_transport_free(transport);
    sentry__run_clean(run, true);
    sentry__run_free(run);
    sentry__path_remove_all(database_path);
    sentry__path_free(database_path);
}

SENTRY_THREAD_FN
thread_worker(void *called)
{
    init_framework(called);

    sentry_set_transaction("demo-trans");

    sentry_capture_event(sentry_value_new_message_event(
        SENTRY_LEVEL_INFO, "root", "Hello World!"));

    sentry_value_t obj = sentry_value_new_object();
    // something that is not a UUID, as this will be forcibly changed
    sentry_value_set_by_key(obj, "event_id", sentry_value_new_int32(1234));
    sentry_capture_event(obj);

    return 0;
}

SENTRY_TEST(concurrent_init)
{
    long called = 0;

#ifdef SENTRY_PLATFORM_NX
#    define THREADS_NUM 90
#else
#    define THREADS_NUM 100
#endif
    sentry_threadid_t threads[THREADS_NUM];

    for (size_t i = 0; i < THREADS_NUM; i++) {
        sentry__thread_init(&threads[i]);
        sentry__thread_spawn(&threads[i], &thread_worker, &called);
    }

    for (size_t i = 0; i < THREADS_NUM; i++) {
        sentry__thread_join(threads[i]);
        sentry__thread_free(&threads[i]);
    }

    sentry_close();

    // 1 session, and up to 2 events per thread.
    // might be less because `capture_event` races with close/init and might
    // lose events.
    TEST_CHECK(called >= THREADS_NUM * 1);
    TEST_CHECK(called <= THREADS_NUM * 3);
}

SENTRY_THREAD_FN
thread_breadcrumb(void *UNUSED(arg))
{
    sentry_value_t breadcrumb = sentry_value_new_breadcrumb("foo", "bar");
    sentry_add_breadcrumb(breadcrumb);

    return 0;
}

SENTRY_TEST(concurrent_uninit)
{
    sentry_value_t user = sentry_value_new_object();
    sentry_set_user(user);

    sentry_threadid_t thread;
    sentry__thread_init(&thread);
    sentry__thread_spawn(&thread, &thread_breadcrumb, NULL);

    sentry_value_t breadcrumb = sentry_value_new_breadcrumb("foo", "bar");
    sentry_add_breadcrumb(breadcrumb);

    sentry__thread_join(thread);
    sentry__thread_free(&thread);

    sentry_close();
}
