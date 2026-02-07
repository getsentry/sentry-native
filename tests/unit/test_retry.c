#include "sentry_database.h"
#include "sentry_envelope.h"
#include "sentry_options.h"
#include "sentry_path.h"
#include "sentry_retry.h"
#include "sentry_sync.h"
#include "sentry_testsupport.h"
#include "sentry_transport.h"
#include "sentry_utils.h"
#include "sentry_uuid.h"
#include "sentry_value.h"

#define NUM_ENVELOPES 3

static uint64_t g_timestamps[NUM_ENVELOPES];
static int g_call_count;

static sentry_send_result_t
recording_retry_func(void *envelope, void *state)
{
    (void)envelope;
    (void)state;
    if (g_call_count < NUM_ENVELOPES) {
        g_timestamps[g_call_count] = sentry__monotonic_time();
    }
    g_call_count++;
    return SENTRY_SEND_SUCCESS;
}

static int
test_submit_delayed(void *_state,
    void (*exec_func)(void *task_data, void *state),
    void (*cleanup_func)(void *task_data), void *task_data, uint64_t delay_ms)
{
    return sentry__bgworker_submit_delayed((sentry_bgworker_t *)_state,
        exec_func, cleanup_func, task_data, delay_ms);
}

SENTRY_TEST(retry_throttle)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_http_retry(options, 5);
    sentry_init(options);

    if (!sentry__transport_can_retry(options->transport)) {
        sentry_close();
        SKIP_TEST();
    }

    sentry__transport_set_retry_func(
        options->transport, test_submit_delayed, recording_retry_func);

    sentry_path_t *retry_path
        = sentry__path_join_str(options->database_path, "retry");
    TEST_ASSERT(!!retry_path);
    sentry__path_remove_all(retry_path);

    g_call_count = 0;
    memset(g_timestamps, 0, sizeof(g_timestamps));

    TEST_ASSERT(sentry__path_create_dir_all(retry_path) == 0);
    for (int i = 0; i < NUM_ENVELOPES; i++) {
        sentry_envelope_t *envelope = sentry__envelope_new();
        TEST_ASSERT(!!envelope);
        sentry_uuid_t event_id = sentry_uuid_new_v4();
        sentry_value_t event = sentry__value_new_event_with_id(&event_id);
        sentry__envelope_add_event(envelope, event);
        char *filename = sentry__uuid_as_filename(&event_id, ".envelope");
        TEST_ASSERT(!!filename);
        sentry_path_t *path = sentry__path_join_str(retry_path, filename);
        sentry_free(filename);
        TEST_ASSERT(!!path);
        TEST_CHECK(sentry_envelope_write_to_path(envelope, path) == 0);
        sentry__path_free(path);
        sentry_envelope_free(envelope);
    }

    sentry_retry_t *retry = sentry__retry_new(options);
    TEST_ASSERT(!!retry);

    uint64_t before = sentry__monotonic_time();
    sentry__retry_process_envelopes(retry);

    sentry_bgworker_t *bgw = sentry__transport_get_bgworker(options->transport);
    TEST_ASSERT(!!bgw);
    for (int i = 0; i < 20 && g_call_count < NUM_ENVELOPES; i++) {
        sentry__bgworker_flush(bgw, 500);
    }

    TEST_CHECK_INT_EQUAL(g_call_count, NUM_ENVELOPES);

    uint64_t initial_delay = g_timestamps[0] - before;
    TEST_CHECK(initial_delay >= SENTRY_RETRY_DELAY_MS);
    TEST_MSG("initial: expected >= %dms, got %llu ms", SENTRY_RETRY_DELAY_MS,
        (unsigned long long)initial_delay);

    for (int i = 1; i < g_call_count && i < NUM_ENVELOPES; i++) {
        uint64_t delta = g_timestamps[i] - g_timestamps[i - 1];
        TEST_CHECK(delta >= SENTRY_RETRY_DELAY_MS);
        TEST_MSG("gap[%d]: expected >= %dms, got %llu ms", i,
            SENTRY_RETRY_DELAY_MS, (unsigned long long)delta);
    }

    sentry__retry_free(retry);
    sentry__path_free(retry_path);
    sentry_close();
}

static int
count_envelope_files(const sentry_path_t *dir)
{
    int count = 0;
    sentry_pathiter_t *iter = sentry__path_iter_directory(dir);
    const sentry_path_t *file;
    while (iter && (file = sentry__pathiter_next(iter)) != NULL) {
        if (sentry__path_ends_with(file, ".envelope")) {
            count++;
        }
    }
    sentry__pathiter_free(iter);
    return count;
}

SENTRY_TEST(retry_cache)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_cache_keep(options, true);
    sentry_options_set_http_retry(options, 5);
    sentry_init(options);

    if (!sentry__transport_can_retry(options->transport)) {
        sentry_close();
        SKIP_TEST();
    }

    sentry_path_t *retry_path
        = sentry__path_join_str(options->database_path, "retry");
    sentry_path_t *cache_path
        = sentry__path_join_str(options->database_path, "cache");
    TEST_ASSERT(!!retry_path && !!cache_path);
    sentry__path_remove_all(retry_path);
    sentry__path_remove_all(cache_path);

    sentry_path_t *old_run_path
        = sentry__path_join_str(options->database_path, "old.run");
    TEST_ASSERT(!!old_run_path);
    TEST_ASSERT(sentry__path_create_dir_all(old_run_path) == 0);

    sentry_envelope_t *envelope = sentry__envelope_new();
    TEST_ASSERT(!!envelope);
    sentry_uuid_t event_id = sentry_uuid_new_v4();
    sentry_value_t event = sentry__value_new_event_with_id(&event_id);
    sentry__envelope_add_event(envelope, event);

    char *envelope_filename = sentry__uuid_as_filename(&event_id, ".envelope");
    TEST_ASSERT(!!envelope_filename);
    sentry_path_t *old_envelope_path
        = sentry__path_join_str(old_run_path, envelope_filename);
    TEST_ASSERT(
        sentry_envelope_write_to_path(envelope, old_envelope_path) == 0);
    sentry_envelope_free(envelope);

    TEST_ASSERT(sentry__path_is_file(old_envelope_path));
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 0);
    TEST_CHECK_INT_EQUAL(count_envelope_files(cache_path), 0);

    sentry__process_old_runs(options, 0);
    sentry_flush(1000);
    TEST_ASSERT(!sentry__path_is_file(old_envelope_path));
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 1);

    sentry_retry_t *retry = sentry__retry_new(options);
    TEST_ASSERT(!!retry);
    for (int i = 0; i < 5; i++) {
        sentry__retry_process_envelopes(retry);
        sentry_flush(1000);
    }
    sentry__retry_free(retry);

    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 0);
    TEST_CHECK_INT_EQUAL(count_envelope_files(cache_path), 1);

    sentry__path_free(old_envelope_path);
    sentry__path_free(old_run_path);
    sentry__path_free(retry_path);
    sentry__path_free(cache_path);
    sentry_free(envelope_filename);
    sentry_close();
}
