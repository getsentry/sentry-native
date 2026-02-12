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

typedef struct {
    sentry_retry_t *retry;
    int status_code;
    size_t count;
} retry_test_ctx_t;

static bool
handle_result_cb(const sentry_path_t *path, void *_ctx)
{
    retry_test_ctx_t *ctx = _ctx;
    ctx->count++;
    sentry__retry_handle_result(ctx->retry, path, ctx->status_code);
    return true;
}

static bool
count_cb(const sentry_path_t *path, void *_count)
{
    (void)path;
    (*(size_t *)_count)++;
    return true;
}

SENTRY_TEST(retry_throttle)
{
    sentry_path_t *db_path
        = sentry__path_from_str(SENTRY_TEST_PATH_PREFIX ".retry-throttle");
    sentry__path_remove_all(db_path);

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_database_path(
        options, SENTRY_TEST_PATH_PREFIX ".retry-throttle");
    sentry_options_set_http_retries(options, 5);
    sentry_retry_t *retry = sentry__retry_new(options);
    sentry_options_free(options);
    TEST_ASSERT(!!retry);

    sentry_path_t *retry_path = sentry__path_join_str(db_path, "retry");

    uint64_t old_ts = (uint64_t)time(NULL) - 10 * SENTRY_RETRY_BACKOFF_BASE_S;
    sentry_uuid_t ids[4];
    for (int i = 0; i < 4; i++) {
        ids[i] = sentry_uuid_new_v4();
        write_retry_file(retry_path, old_ts, 0, &ids[i]);
    }

    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 4);

    retry_test_ctx_t ctx = { retry, 200, 0 };
    sentry__retry_foreach(retry, false, handle_result_cb, &ctx);
    TEST_CHECK_INT_EQUAL(ctx.count, 4);
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 0);

    sentry__retry_free(retry);
    sentry__path_free(retry_path);
    sentry__path_remove_all(db_path);
    sentry__path_free(db_path);
}

SENTRY_TEST(retry_result)
{
    sentry_path_t *db_path
        = sentry__path_from_str(SENTRY_TEST_PATH_PREFIX ".retry-result");
    sentry__path_remove_all(db_path);

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_database_path(
        options, SENTRY_TEST_PATH_PREFIX ".retry-result");
    sentry_options_set_http_retries(options, 2);
    sentry_retry_t *retry = sentry__retry_new(options);
    sentry_options_free(options);
    TEST_ASSERT(!!retry);

    sentry_path_t *retry_path = sentry__path_join_str(db_path, "retry");

    uint64_t old_ts = (uint64_t)time(NULL) - 10 * SENTRY_RETRY_BACKOFF_BASE_S;
    sentry_uuid_t event_id = sentry_uuid_new_v4();

    // 1. Success (200) → removes from retry dir
    write_retry_file(retry_path, old_ts, 0, &event_id);
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 1);
    TEST_CHECK_INT_EQUAL(find_envelope_attempt(retry_path), 0);

    retry_test_ctx_t ctx = { retry, 200, 0 };
    sentry__retry_foreach(retry, false, handle_result_cb, &ctx);
    TEST_CHECK_INT_EQUAL(ctx.count, 1);
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 0);

    // 2. Rate limited (429) → removes
    write_retry_file(retry_path, old_ts, 0, &event_id);
    ctx = (retry_test_ctx_t) { retry, 429, 0 };
    sentry__retry_foreach(retry, false, handle_result_cb, &ctx);
    TEST_CHECK_INT_EQUAL(ctx.count, 1);
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 0);

    // 3. Discard (0) → removes
    write_retry_file(retry_path, old_ts, 0, &event_id);
    ctx = (retry_test_ctx_t) { retry, 0, 0 };
    sentry__retry_foreach(retry, false, handle_result_cb, &ctx);
    TEST_CHECK_INT_EQUAL(ctx.count, 1);
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 0);

    // 4. Network error → bumps count
    write_retry_file(retry_path, old_ts, 0, &event_id);
    TEST_CHECK_INT_EQUAL(find_envelope_attempt(retry_path), 0);

    ctx = (retry_test_ctx_t) { retry, -1, 0 };
    sentry__retry_foreach(retry, false, handle_result_cb, &ctx);
    TEST_CHECK_INT_EQUAL(ctx.count, 1);
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 1);
    TEST_CHECK_INT_EQUAL(find_envelope_attempt(retry_path), 1);

    // 5. Network error at max count → exceeds max_retries=2, removed
    sentry__path_remove_all(retry_path);
    sentry__path_create_dir_all(retry_path);
    write_retry_file(retry_path, old_ts, 1, &event_id);
    ctx = (retry_test_ctx_t) { retry, -1, 0 };
    sentry__retry_foreach(retry, false, handle_result_cb, &ctx);
    TEST_CHECK_INT_EQUAL(ctx.count, 1);
    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 0);

    sentry__retry_free(retry);
    sentry__path_free(retry_path);
    sentry__path_remove_all(db_path);
    sentry__path_free(db_path);
}

SENTRY_TEST(retry_session)
{
    SENTRY_TEST_OPTIONS_NEW(init_options);
    sentry_options_set_dsn(init_options, "https://foo@sentry.invalid/42");
    sentry_options_set_release(init_options, "test@1.0.0");
    sentry_init(init_options);

    sentry_path_t *db_path
        = sentry__path_from_str(SENTRY_TEST_PATH_PREFIX ".retry-session");
    sentry__path_remove_all(db_path);

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_database_path(
        options, SENTRY_TEST_PATH_PREFIX ".retry-session");
    sentry_options_set_http_retries(options, 2);
    sentry_retry_t *retry = sentry__retry_new(options);
    sentry_options_free(options);
    TEST_ASSERT(!!retry);

    sentry_path_t *retry_path = sentry__path_join_str(db_path, "retry");

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
    sentry__path_free(retry_path);
    sentry__path_remove_all(db_path);
    sentry__path_free(db_path);
    sentry_close();
}

SENTRY_TEST(retry_cache)
{
    sentry_path_t *db_path
        = sentry__path_from_str(SENTRY_TEST_PATH_PREFIX ".retry-cache");
    sentry__path_remove_all(db_path);

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_database_path(
        options, SENTRY_TEST_PATH_PREFIX ".retry-cache");
    sentry_options_set_http_retries(options, 5);
    sentry_options_set_cache_keep(options, 1);
    sentry_retry_t *retry = sentry__retry_new(options);
    sentry_options_free(options);
    TEST_ASSERT(!!retry);

    sentry_path_t *retry_path = sentry__path_join_str(db_path, "retry");
    sentry_path_t *cache_path = sentry__path_join_str(db_path, "cache");

    uint64_t old_ts = (uint64_t)time(NULL) - 10 * SENTRY_RETRY_BACKOFF_BASE_S;
    sentry_uuid_t event_id = sentry_uuid_new_v4();
    write_retry_file(retry_path, old_ts, 4, &event_id);

    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 1);
    TEST_CHECK_INT_EQUAL(count_envelope_files(cache_path), 0);

    // Network error on a file at count=4 with max_retries=5 → moves to cache
    retry_test_ctx_t ctx = { retry, -1, 0 };
    sentry__retry_foreach(retry, false, handle_result_cb, &ctx);
    TEST_CHECK_INT_EQUAL(ctx.count, 1);

    TEST_CHECK_INT_EQUAL(count_envelope_files(retry_path), 0);
    TEST_CHECK_INT_EQUAL(count_envelope_files(cache_path), 1);

    sentry__retry_free(retry);
    sentry__path_free(retry_path);
    sentry__path_free(cache_path);
    sentry__path_remove_all(db_path);
    sentry__path_free(db_path);
}

SENTRY_TEST(retry_backoff)
{
    sentry_path_t *db_path
        = sentry__path_from_str(SENTRY_TEST_PATH_PREFIX ".retry-backoff");
    sentry__path_remove_all(db_path);

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_database_path(
        options, SENTRY_TEST_PATH_PREFIX ".retry-backoff");
    sentry_options_set_http_retries(options, 5);
    sentry_retry_t *retry = sentry__retry_new(options);
    sentry_options_free(options);
    TEST_ASSERT(!!retry);

    sentry_path_t *retry_path = sentry__path_join_str(db_path, "retry");

    uint64_t base = SENTRY_RETRY_BACKOFF_BASE_S;
    uint64_t ref = (uint64_t)time(NULL) - 10 * base;

    // retry 0: 10*base old, eligible (backoff=base)
    sentry_uuid_t id1 = sentry_uuid_new_v4();
    write_retry_file(retry_path, ref, 0, &id1);

    // retry 1: 1*base old, not yet eligible (backoff=2*base)
    sentry_uuid_t id2 = sentry_uuid_new_v4();
    write_retry_file(retry_path, ref + 9 * base, 1, &id2);

    // retry 1: 10*base old, eligible (backoff=2*base)
    sentry_uuid_t id3 = sentry_uuid_new_v4();
    write_retry_file(retry_path, ref, 1, &id3);

    // retry 2: 2*base old, not eligible (backoff=4*base)
    sentry_uuid_t id4 = sentry_uuid_new_v4();
    write_retry_file(retry_path, ref + 8 * base, 2, &id4);

    // Startup scan (no backoff check): all 4 files returned
    size_t count = 0;
    sentry__retry_foreach(retry, true, count_cb, &count);
    TEST_CHECK_INT_EQUAL(count, 4);

    // With backoff check: only eligible ones (id1 and id3)
    count = 0;
    sentry__retry_foreach(retry, false, count_cb, &count);
    TEST_CHECK_INT_EQUAL(count, 2);

    // Verify backoff calculation
    TEST_CHECK_UINT64_EQUAL(sentry__retry_backoff(0), base);
    TEST_CHECK_UINT64_EQUAL(sentry__retry_backoff(1), base * 2);
    TEST_CHECK_UINT64_EQUAL(sentry__retry_backoff(2), base * 4);
    TEST_CHECK_UINT64_EQUAL(sentry__retry_backoff(3), base * 8);
    TEST_CHECK_UINT64_EQUAL(sentry__retry_backoff(4), base * 8);

    sentry__retry_free(retry);
    sentry__path_free(retry_path);
    sentry__path_remove_all(db_path);
    sentry__path_free(db_path);
}

SENTRY_TEST(retry_no_duplicate_rescan)
{
    sentry_path_t *db_path
        = sentry__path_from_str(SENTRY_TEST_PATH_PREFIX ".retry-no-dup-rescan");
    sentry__path_remove_all(db_path);

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_database_path(
        options, SENTRY_TEST_PATH_PREFIX ".retry-no-dup-rescan");
    sentry_options_set_http_retries(options, 3);
    sentry_retry_t *retry = sentry__retry_new(options);
    sentry_options_free(options);
    TEST_ASSERT(!!retry);

    sentry_path_t *retry_path = sentry__path_join_str(db_path, "retry");

    uint64_t old_ts = (uint64_t)time(NULL) - 10 * SENTRY_RETRY_BACKOFF_BASE_S;
    sentry_uuid_t event_id = sentry_uuid_new_v4();
    write_retry_file(retry_path, old_ts, 0, &event_id);

    // First scan returns the file
    retry_test_ctx_t ctx = { retry, 200, 0 };
    sentry__retry_foreach(retry, false, handle_result_cb, &ctx);
    TEST_CHECK_INT_EQUAL(ctx.count, 1);

    // Second scan returns nothing
    ctx.count = 0;
    sentry__retry_foreach(retry, false, handle_result_cb, &ctx);
    TEST_CHECK_INT_EQUAL(ctx.count, 0);

    sentry__path_free(retry_path);
    sentry__retry_free(retry);
    sentry__path_remove_all(db_path);
    sentry__path_free(db_path);
}
