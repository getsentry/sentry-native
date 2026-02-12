#include "sentry_envelope.h"
#include "sentry_path.h"
#include "sentry_retry.h"
#include "sentry_session.h"
#include "sentry_testsupport.h"
#include "sentry_utils.h"
#include "sentry_uuid.h"

#include <string.h>
#include <time.h>

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
        uint64_t ts;
        int attempt;
        const char *uuid;
        if (sentry__retry_parse_filename(name, &ts, &attempt, &uuid)) {
            sentry__pathiter_free(iter);
            return attempt;
        }
    }
    sentry__pathiter_free(iter);
    return -1;
}

static void
write_retry_file(const sentry_path_t *retry_path, uint64_t timestamp,
    int retry_count, const sentry_uuid_t *event_id)
{
    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry_value_t event = sentry__value_new_event_with_id(event_id);
    sentry__envelope_add_event(envelope, event);

    char uuid_str[37];
    sentry_uuid_as_string(event_id, uuid_str);
    char filename[80];
    snprintf(filename, sizeof(filename), "%llu-%02d-%s.envelope",
        (unsigned long long)timestamp, retry_count, uuid_str);

    sentry_path_t *path = sentry__path_join_str(retry_path, filename);
    (void)sentry_envelope_write_to_path(envelope, path);
    sentry__path_free(path);
    sentry_envelope_free(envelope);
}

static sentry_envelope_t *
make_test_envelope(sentry_uuid_t *event_id)
{
    *event_id = sentry_uuid_new_v4();
    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry_value_t event = sentry__value_new_event_with_id(event_id);
    sentry__envelope_add_event(envelope, event);
    return envelope;
}

SENTRY_TEST(retry_throttle)
{
    sentry_path_t *retry_path
        = sentry__path_from_str(SENTRY_TEST_PATH_PREFIX ".retry-throttle");
    sentry__path_remove_all(retry_path);
    sentry__path_create_dir_all(retry_path);

    sentry_retry_t *retry = sentry__retry_new(retry_path, NULL, 5);
    TEST_ASSERT(!!retry);

    sentry_uuid_t ids[4];
    for (int i = 0; i < 4; i++) {
        sentry_envelope_t *envelope = make_test_envelope(&ids[i]);
        sentry__retry_write_envelope(retry, envelope);
        sentry_envelope_free(envelope);
    }

    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 4);

    size_t count = 0;
    sentry_path_t **paths = sentry__retry_scan(retry, true, &count);
    TEST_CHECK_INT_EQUAL(count, 4);

    for (size_t i = 0; i < count; i++) {
        sentry__retry_handle_result(retry, paths[i], 200);
    }
    sentry__retry_free_paths(paths, count);
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 0);

    sentry__retry_free(retry);
    sentry__path_remove_all(retry_path);
    sentry__path_free(retry_path);
}

SENTRY_TEST(retry_result)
{
    sentry_path_t *retry_path
        = sentry__path_from_str(SENTRY_TEST_PATH_PREFIX ".retry-result");
    sentry__path_remove_all(retry_path);
    sentry__path_create_dir_all(retry_path);

    sentry_retry_t *retry = sentry__retry_new(retry_path, NULL, 2);
    TEST_ASSERT(!!retry);

    sentry_uuid_t event_id;
    sentry_envelope_t *envelope = make_test_envelope(&event_id);

    // 1. Write envelope (simulates network error → save for retry)
    sentry__retry_write_envelope(retry, envelope);
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 1);
    TEST_CHECK_INT_EQUAL(find_envelope_attempt(retry_path), 0);

    // 2. Success (200) → removes from retry dir
    size_t count = 0;
    sentry_path_t **paths = sentry__retry_scan(retry, true, &count);
    TEST_CHECK_INT_EQUAL(count, 1);
    sentry__retry_handle_result(retry, paths[0], 200);
    sentry__retry_free_paths(paths, count);
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 0);

    // 3. Write again
    sentry__retry_write_envelope(retry, envelope);
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 1);

    // 4. Rate limited (429) → removes
    paths = sentry__retry_scan(retry, true, &count);
    TEST_CHECK_INT_EQUAL(count, 1);
    sentry__retry_handle_result(retry, paths[0], 429);
    sentry__retry_free_paths(paths, count);
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 0);

    // 5. Write again, then discard (0) → removes
    sentry__retry_write_envelope(retry, envelope);
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 1);
    paths = sentry__retry_scan(retry, true, &count);
    TEST_CHECK_INT_EQUAL(count, 1);
    sentry__retry_handle_result(retry, paths[0], 0);
    sentry__retry_free_paths(paths, count);
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 0);

    // 6. Network error twice → bumps count
    sentry__retry_write_envelope(retry, envelope);
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 1);
    TEST_CHECK_INT_EQUAL(find_envelope_attempt(retry_path), 0);

    paths = sentry__retry_scan(retry, true, &count);
    TEST_CHECK_INT_EQUAL(count, 1);
    sentry__retry_handle_result(retry, paths[0], -1);
    sentry__retry_free_paths(paths, count);
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 1);
    TEST_CHECK_INT_EQUAL(find_envelope_attempt(retry_path), 1);

    // 7. Network error again → exceeds max_retries=2, removed
    paths = sentry__retry_scan(retry, true, &count);
    TEST_CHECK_INT_EQUAL(count, 1);
    sentry__retry_handle_result(retry, paths[0], -1);
    sentry__retry_free_paths(paths, count);
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 0);

    sentry_envelope_free(envelope);
    sentry__retry_free(retry);
    sentry__path_remove_all(retry_path);
    sentry__path_free(retry_path);
}

SENTRY_TEST(retry_session)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_release(options, "test@1.0.0");
    sentry_init(options);

    sentry_path_t *retry_path
        = sentry__path_from_str(SENTRY_TEST_PATH_PREFIX ".retry-session");
    sentry__path_remove_all(retry_path);
    sentry__path_create_dir_all(retry_path);

    sentry_retry_t *retry = sentry__retry_new(retry_path, NULL, 2);
    TEST_ASSERT(!!retry);

    sentry_session_t *session = sentry__session_new();
    TEST_ASSERT(!!session);
    sentry_envelope_t *envelope = sentry__envelope_new();
    TEST_ASSERT(!!envelope);
    sentry__envelope_add_session(envelope, session);

    // Session-only envelopes have no event_id → should not be written
    sentry__retry_write_envelope(retry, envelope);
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 0);

    sentry_envelope_free(envelope);
    sentry__session_free(session);
    sentry__retry_free(retry);
    sentry__path_remove_all(retry_path);
    sentry__path_free(retry_path);
    sentry_close();
}

SENTRY_TEST(retry_cache)
{
    sentry_path_t *retry_path
        = sentry__path_from_str(SENTRY_TEST_PATH_PREFIX ".retry-cache/retry");
    sentry_path_t *cache_path
        = sentry__path_from_str(SENTRY_TEST_PATH_PREFIX ".retry-cache/cache");
    sentry__path_remove_all(retry_path);
    sentry__path_remove_all(cache_path);
    sentry__path_create_dir_all(retry_path);
    sentry__path_create_dir_all(cache_path);

    sentry_retry_t *retry = sentry__retry_new(retry_path, cache_path, 5);
    TEST_ASSERT(!!retry);

    // Create a retry file at the max retry count (4, with max_retries=5)
    sentry_uuid_t event_id = sentry_uuid_new_v4();
    write_retry_file(retry_path, sentry__monotonic_time(), 4, &event_id);

    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 1);
    TEST_CHECK_INT_EQUAL(count_envelope_files(cache_path), 0);

    // Network error on a file at count=4 with max_retries=5 → moves to cache
    size_t count = 0;
    sentry_path_t **paths = sentry__retry_scan(retry, true, &count);
    TEST_CHECK_INT_EQUAL(count, 1);
    sentry__retry_handle_result(retry, paths[0], -1);
    sentry__retry_free_paths(paths, count);

    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 0);
    TEST_CHECK_INT_EQUAL(count_envelope_files(cache_path), 1);

    sentry__retry_free(retry);
    sentry__path_remove_all(retry_path);
    sentry__path_remove_all(cache_path);
    sentry__path_free(retry_path);
    sentry__path_free(cache_path);
}

SENTRY_TEST(retry_backoff)
{
    sentry_path_t *retry_path
        = sentry__path_from_str(SENTRY_TEST_PATH_PREFIX ".retry-backoff");
    sentry__path_remove_all(retry_path);
    sentry__path_create_dir_all(retry_path);

    sentry_retry_t *retry = sentry__retry_new(retry_path, NULL, 5);
    TEST_ASSERT(!!retry);

    uint64_t now = sentry__monotonic_time();
    uint64_t base = SENTRY_RETRY_BACKOFF_BASE_MS;

    // retry 0 with old timestamp: eligible (base backoff expired)
    sentry_uuid_t id1 = sentry_uuid_new_v4();
    write_retry_file(retry_path, now - base, 0, &id1);

    // retry 1 with recent timestamp: not yet eligible (needs 2*base)
    sentry_uuid_t id2 = sentry_uuid_new_v4();
    write_retry_file(retry_path, now, 1, &id2);

    // retry 1 with old timestamp: eligible (2*base backoff expired)
    sentry_uuid_t id3 = sentry_uuid_new_v4();
    write_retry_file(retry_path, now - 2 * base, 1, &id3);

    // retry 2 with old-ish timestamp: needs 4*base but only 2*base old
    sentry_uuid_t id4 = sentry_uuid_new_v4();
    write_retry_file(retry_path, now - 2 * base, 2, &id4);

    // Startup scan (no backoff check): all 4 files returned
    size_t count = 0;
    sentry_path_t **paths = sentry__retry_scan(retry, true, &count);
    TEST_CHECK_INT_EQUAL(count, 4);
    sentry__retry_free_paths(paths, count);

    // With backoff check: only eligible ones (id1 and id3)
    paths = sentry__retry_scan(retry, false, &count);
    TEST_CHECK_INT_EQUAL(count, 2);
    sentry__retry_free_paths(paths, count);

    // Verify backoff_ms calculation
    TEST_CHECK_UINT64_EQUAL(sentry__retry_backoff_ms(0), base);
    TEST_CHECK_UINT64_EQUAL(sentry__retry_backoff_ms(1), base * 2);
    TEST_CHECK_UINT64_EQUAL(sentry__retry_backoff_ms(2), base * 4);
    TEST_CHECK_UINT64_EQUAL(sentry__retry_backoff_ms(3), base * 8);
    TEST_CHECK_UINT64_EQUAL(sentry__retry_backoff_ms(4), base * 8);

    sentry__retry_free(retry);
    sentry__path_remove_all(retry_path);
    sentry__path_free(retry_path);
}

SENTRY_TEST(retry_no_duplicate_rescan)
{
    sentry_path_t *retry_path
        = sentry__path_from_str(SENTRY_TEST_PATH_PREFIX ".retry-no-dup-rescan");
    sentry__path_remove_all(retry_path);
    sentry__path_create_dir_all(retry_path);

    sentry_retry_t *retry = sentry__retry_new(retry_path, NULL, 3);
    TEST_ASSERT(!!retry);

    sentry_uuid_t event_id;
    sentry_envelope_t *envelope = make_test_envelope(&event_id);
    sentry__retry_write_envelope(retry, envelope);

    // First scan returns the file
    size_t count = 0;
    sentry_path_t **paths = sentry__retry_scan(retry, true, &count);
    TEST_CHECK_INT_EQUAL(count, 1);

    // Handle as success → removes from retry dir
    sentry__retry_handle_result(retry, paths[0], 200);
    sentry__retry_free_paths(paths, count);

    // Second scan returns nothing
    paths = sentry__retry_scan(retry, true, &count);
    TEST_CHECK_INT_EQUAL(count, 0);
    sentry__retry_free_paths(paths, count);

    TEST_CHECK(!sentry__retry_has_files(retry));

    sentry_envelope_free(envelope);
    sentry__retry_free(retry);
    sentry__path_remove_all(retry_path);
    sentry__path_free(retry_path);
}
