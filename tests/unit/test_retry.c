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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NUM_ENVELOPES 4

static uint64_t g_timestamps[NUM_ENVELOPES];
static int g_call_count;

static sentry_send_result_t
test_send(void *envelope, void *state)
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
test_schedule(void *_state, void (*exec_func)(void *task_data, void *state),
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
        options->transport, test_schedule, test_send);

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

static int
find_envelope_attempt(const sentry_path_t *dir)
{
    sentry_pathiter_t *iter = sentry__path_iter_directory(dir);
    const sentry_path_t *file;
    while (iter && (file = sentry__pathiter_next(iter)) != NULL) {
        if (!sentry__path_ends_with(file, ".envelope")) {
            continue;
        }
        const char *name = sentry__path_filename(file);
        const char *first_dash = strchr(name, '-');
        if (first_dash) {
            int attempt = atoi(first_dash + 1);
            sentry__pathiter_free(iter);
            return attempt;
        }
    }
    sentry__pathiter_free(iter);
    return 0;
}

SENTRY_TEST(retry_result)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_http_retry(options, 2);
    sentry_init(options);

    sentry_path_t *retry_path
        = sentry__path_join_str(options->database_path, "retry");
    TEST_ASSERT(!!retry_path);
    sentry__path_remove_all(retry_path);

    sentry_retry_t *retry = sentry__retry_new(options);
    TEST_ASSERT(!!retry);

    sentry_envelope_t *envelope = sentry__envelope_new();
    TEST_ASSERT(!!envelope);
    sentry_uuid_t event_id = sentry_uuid_new_v4();
    sentry_value_t event = sentry__value_new_event_with_id(&event_id);
    sentry__envelope_add_event(envelope, event);

    // 1. NETWORK_ERROR → writes to retry dir
    sentry__retry_process_result(retry, envelope, SENTRY_SEND_NETWORK_ERROR);
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 1);
    TEST_CHECK_INT_EQUAL(find_envelope_attempt(retry_path), 1);

    // 2. SUCCESS → removes from retry dir
    sentry__retry_process_result(retry, envelope, SENTRY_SEND_SUCCESS);
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 0);

    // 3. NETWORK_ERROR → re-creates in retry dir
    sentry__retry_process_result(retry, envelope, SENTRY_SEND_NETWORK_ERROR);
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 1);

    // 4. RATE_LIMITED → removes from retry dir
    sentry__retry_process_result(retry, envelope, SENTRY_SEND_RATE_LIMITED);
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 0);

    // 5. NETWORK_ERROR x2 → attempt bumps to 2
    sentry__retry_process_result(retry, envelope, SENTRY_SEND_NETWORK_ERROR);
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 1);
    TEST_CHECK_INT_EQUAL(find_envelope_attempt(retry_path), 1);

    sentry__retry_process_result(retry, envelope, SENTRY_SEND_NETWORK_ERROR);
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 1);
    TEST_CHECK_INT_EQUAL(find_envelope_attempt(retry_path), 2);

    // 6. NETWORK_ERROR again → exceeds max_attempts=2, discarded
    sentry__retry_process_result(retry, envelope, SENTRY_SEND_NETWORK_ERROR);
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 0);

    sentry_envelope_free(envelope);
    sentry__retry_free(retry);
    sentry__path_free(retry_path);
    sentry_close();
}

static void
write_retry_file(const sentry_path_t *retry_path, time_t timestamp, int attempt,
    const sentry_uuid_t *event_id)
{
    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry_value_t event = sentry__value_new_event_with_id(event_id);
    sentry__envelope_add_event(envelope, event);

    char uuid_str[37];
    sentry_uuid_as_string(event_id, uuid_str);
    char filename[80];
    snprintf(filename, sizeof(filename), "%llu-%02d-%s.envelope",
        (unsigned long long)timestamp, attempt, uuid_str);

    sentry_path_t *path = sentry__path_join_str(retry_path, filename);
    (void)sentry_envelope_write_to_path(envelope, path);
    sentry__path_free(path);
    sentry_envelope_free(envelope);
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
    TEST_ASSERT(sentry__path_create_dir_all(retry_path) == 0);

    // Create a retry file at the max attempt. Startup retries all files,
    // the transport fails to send, retry_process_result bumps the attempt
    // past max_attempts and moves to cache.
    sentry_uuid_t event_id = sentry_uuid_new_v4();
    write_retry_file(retry_path, time(NULL), 5, &event_id);

    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 1);
    TEST_CHECK_INT_EQUAL(count_envelope_files(cache_path), 0);

    sentry_retry_t *retry = sentry__retry_new(options);
    TEST_ASSERT(!!retry);
    sentry__retry_process_envelopes(retry);
    sentry_flush(1000);
    sentry__retry_free(retry);

    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 0);
    TEST_CHECK_INT_EQUAL(count_envelope_files(cache_path), 1);

    sentry__path_free(retry_path);
    sentry__path_free(cache_path);
    sentry_close();
}

SENTRY_TEST(retry_backoff)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_http_retry(options, 5);

    sentry_path_t *retry_path
        = sentry__path_join_str(options->database_path, "retry");
    TEST_ASSERT(!!retry_path);
    sentry__path_remove_all(retry_path);

    sentry_init(options);

    if (!sentry__transport_can_retry(options->transport)) {
        sentry__path_free(retry_path);
        sentry_close();
        SKIP_TEST();
    }

    sentry__transport_set_retry_func(
        options->transport, test_schedule, test_send);
    sentry_flush(5000);

    sentry__path_remove_all(retry_path);
    TEST_ASSERT(sentry__path_create_dir_all(retry_path) == 0);

    g_call_count = 0;
    memset(g_timestamps, 0, sizeof(g_timestamps));

    time_t now = time(NULL);

    sentry_uuid_t id1 = sentry_uuid_new_v4();
    write_retry_file(retry_path, now, 1, &id1);

    // attempt 2 with recent timestamp: not yet eligible for re-scan
    sentry_uuid_t id2 = sentry_uuid_new_v4();
    write_retry_file(retry_path, now, 2, &id2);

    // attempt 2 with old timestamp: eligible for re-scan
    sentry_uuid_t id3 = sentry_uuid_new_v4();
    write_retry_file(retry_path, now - SENTRY_RETRY_BASE_DELAY_S, 2, &id3);

    // attempt 3 with old-ish timestamp: needs 30min but only 15min old
    sentry_uuid_t id4 = sentry_uuid_new_v4();
    write_retry_file(retry_path, now - SENTRY_RETRY_BASE_DELAY_S, 3, &id4);

    sentry_retry_t *retry = sentry__retry_new(options);
    TEST_ASSERT(!!retry);

    // startup retries ALL files regardless of backoff
    sentry__retry_process_envelopes(retry);

    sentry_bgworker_t *bgw = sentry__transport_get_bgworker(options->transport);
    TEST_ASSERT(!!bgw);
    for (int i = 0; i < 20 && g_call_count < 4; i++) {
        sentry__bgworker_flush(bgw, 500);
    }
    TEST_CHECK_INT_EQUAL(g_call_count, 4);

    // re-create files and test re-scan with backoff
    sentry__path_remove_all(retry_path);
    TEST_ASSERT(sentry__path_create_dir_all(retry_path) == 0);

    g_call_count = 0;
    memset(g_timestamps, 0, sizeof(g_timestamps));

    write_retry_file(retry_path, now, 1, &id1);
    write_retry_file(retry_path, now, 2, &id2);
    write_retry_file(retry_path, now - SENTRY_RETRY_BASE_DELAY_S, 2, &id3);
    write_retry_file(retry_path, now - SENTRY_RETRY_BASE_DELAY_S, 3, &id4);

    // re-scan applies backoff: only attempt 1 + old attempt 2 are eligible
    sentry__retry_rescan_envelopes(retry);

    for (int i = 0; i < 20 && g_call_count < 2; i++) {
        sentry__bgworker_flush(bgw, 500);
    }
    TEST_CHECK_INT_EQUAL(g_call_count, 2);

    sentry__retry_free(retry);
    sentry__path_free(retry_path);
    sentry_close();
}
