#include "sentry_database.h"
#include "sentry_envelope.h"
#include "sentry_options.h"
#include "sentry_path.h"
#include "sentry_retry.h"
#include "sentry_session.h"
#include "sentry_testsupport.h"
#include "sentry_transport.h"
#include "sentry_utils.h"
#include "sentry_uuid.h"

#include <string.h>

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
write_retry_file(sentry_retry_t *retry, uint64_t timestamp, int retry_count,
    const sentry_uuid_t *event_id)
{
    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry_value_t event = sentry__value_new_event_with_id(event_id);
    sentry__envelope_add_event(envelope, event);

    char uuid[37];
    sentry_uuid_as_string(event_id, uuid);

    sentry_path_t *path
        = sentry__retry_make_path(retry, timestamp, retry_count, uuid);
    (void)sentry_envelope_write_to_path(envelope, path);
    sentry__path_free(path);
    sentry_envelope_free(envelope);
}

typedef struct {
    int status_code;
    size_t count;
} retry_test_ctx_t;

static int
test_send_cb(sentry_envelope_t *envelope, void *_ctx)
{
    (void)envelope;
    retry_test_ctx_t *ctx = _ctx;
    ctx->count++;
    return ctx->status_code;
}

SENTRY_TEST(retry_filename)
{
    uint64_t ts;
    int count;
    const char *uuid;

    TEST_CHECK(sentry__retry_parse_filename(
        "1234567890-00-abcdefab-1234-5678-9abc-def012345678.envelope", &ts,
        &count, &uuid));
    TEST_CHECK_UINT64_EQUAL(ts, 1234567890);
    TEST_CHECK_INT_EQUAL(count, 0);
    TEST_CHECK(strncmp(uuid, "abcdefab-1234-5678-9abc-def012345678", 36) == 0);

    TEST_CHECK(sentry__retry_parse_filename(
        "999-04-abcdefab-1234-5678-9abc-def012345678.envelope", &ts, &count,
        &uuid));
    TEST_CHECK_UINT64_EQUAL(ts, 999);
    TEST_CHECK_INT_EQUAL(count, 4);

    // negative count
    TEST_CHECK(!sentry__retry_parse_filename(
        "123--01-abcdefab-1234-5678-9abc-def012345678.envelope", &ts, &count,
        &uuid));

    // cache filename (no timestamp/count)
    TEST_CHECK(!sentry__retry_parse_filename(
        "abcdefab-1234-5678-9abc-def012345678.envelope", &ts, &count, &uuid));

    // missing .envelope suffix
    TEST_CHECK(!sentry__retry_parse_filename(
        "123-00-abcdefab-1234-5678-9abc-def012345678.txt", &ts, &count, &uuid));
}

SENTRY_TEST(retry_throttle)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_http_retry(options, true);
    sentry_init(options);

    sentry_retry_t *retry = sentry__retry_new(options);
    TEST_ASSERT(!!retry);

    sentry__path_remove_all(options->run->cache_path);
    sentry__path_create_dir_all(options->run->cache_path);

    uint64_t old_ts
        = sentry__usec_time() / 1000 - 10 * sentry__retry_backoff(0);
    sentry_uuid_t ids[4];
    for (int i = 0; i < 4; i++) {
        ids[i] = sentry_uuid_new_v4();
        write_retry_file(retry, old_ts, 0, &ids[i]);
    }

    TEST_CHECK_INT_EQUAL(count_envelope_files(options->run->cache_path), 4);

    retry_test_ctx_t ctx = { 200, 0 };
    sentry__retry_send(retry, 0, test_send_cb, &ctx);
    TEST_CHECK_INT_EQUAL(ctx.count, 4);
    TEST_CHECK_INT_EQUAL(count_envelope_files(options->run->cache_path), 0);

    sentry__retry_free(retry);
    sentry_close();
}

SENTRY_TEST(retry_result)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_http_retry(options, true);
    sentry_init(options);

    sentry_retry_t *retry = sentry__retry_new(options);
    TEST_ASSERT(!!retry);

    const sentry_path_t *cache_path = options->run->cache_path;
    sentry__path_remove_all(cache_path);
    sentry__path_create_dir_all(cache_path);

    uint64_t old_ts
        = sentry__usec_time() / 1000 - 10 * sentry__retry_backoff(0);
    sentry_uuid_t event_id = sentry_uuid_new_v4();

    // 1. Success (200) → removes
    write_retry_file(retry, old_ts, 0, &event_id);
    TEST_CHECK_INT_EQUAL(count_envelope_files(cache_path), 1);
    TEST_CHECK_INT_EQUAL(find_envelope_attempt(cache_path), 0);

    retry_test_ctx_t ctx = { 200, 0 };
    sentry__retry_send(retry, 0, test_send_cb, &ctx);
    TEST_CHECK_INT_EQUAL(ctx.count, 1);
    TEST_CHECK_INT_EQUAL(count_envelope_files(cache_path), 0);

    // 2. Rate limited (429) → removes
    write_retry_file(retry, old_ts, 0, &event_id);
    ctx = (retry_test_ctx_t) { 429, 0 };
    sentry__retry_send(retry, 0, test_send_cb, &ctx);
    TEST_CHECK_INT_EQUAL(ctx.count, 1);
    TEST_CHECK_INT_EQUAL(count_envelope_files(cache_path), 0);

    // 3. Discard (0) → removes
    write_retry_file(retry, old_ts, 0, &event_id);
    ctx = (retry_test_ctx_t) { 0, 0 };
    sentry__retry_send(retry, 0, test_send_cb, &ctx);
    TEST_CHECK_INT_EQUAL(ctx.count, 1);
    TEST_CHECK_INT_EQUAL(count_envelope_files(cache_path), 0);

    // 4. Network error → bumps count
    write_retry_file(retry, old_ts, 0, &event_id);
    TEST_CHECK_INT_EQUAL(find_envelope_attempt(cache_path), 0);

    ctx = (retry_test_ctx_t) { -1, 0 };
    sentry__retry_send(retry, 0, test_send_cb, &ctx);
    TEST_CHECK_INT_EQUAL(ctx.count, 1);
    TEST_CHECK_INT_EQUAL(count_envelope_files(cache_path), 1);
    TEST_CHECK_INT_EQUAL(find_envelope_attempt(cache_path), 1);

    // 5. Network error at last attempt → removed
    sentry__path_remove_all(cache_path);
    sentry__path_create_dir_all(cache_path);
    uint64_t very_old_ts
        = sentry__usec_time() / 1000 - 2 * sentry__retry_backoff(4);
    write_retry_file(retry, very_old_ts, 4, &event_id);
    ctx = (retry_test_ctx_t) { -1, 0 };
    sentry__retry_send(retry, 0, test_send_cb, &ctx);
    TEST_CHECK_INT_EQUAL(ctx.count, 1);
    TEST_CHECK_INT_EQUAL(count_envelope_files(cache_path), 0);

    sentry__retry_free(retry);
    sentry_close();
}

SENTRY_TEST(retry_session)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_release(options, "test@1.0.0");
    sentry_options_set_http_retry(options, true);
    sentry_init(options);

    sentry_retry_t *retry = sentry__retry_new(options);
    TEST_ASSERT(!!retry);

    sentry__path_remove_all(options->run->cache_path);
    sentry__path_create_dir_all(options->run->cache_path);

    sentry_session_t *session = sentry__session_new();
    TEST_ASSERT(!!session);
    sentry_envelope_t *envelope = sentry__envelope_new();
    TEST_ASSERT(!!envelope);
    sentry__envelope_add_session(envelope, session);

    // Session-only envelopes have no event_id → should not be written
    sentry__retry_write_envelope(retry, envelope);
    TEST_CHECK_INT_EQUAL(count_envelope_files(options->run->cache_path), 0);

    sentry_envelope_free(envelope);
    sentry__session_free(session);
    sentry__retry_free(retry);
    sentry_close();
}

SENTRY_TEST(retry_cache)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_http_retry(options, true);
    sentry_options_set_cache_keep(options, 1);
    sentry_init(options);

    sentry_retry_t *retry = sentry__retry_new(options);
    TEST_ASSERT(!!retry);

    const sentry_path_t *cache_path = options->run->cache_path;
    sentry__path_remove_all(cache_path);
    sentry__path_create_dir_all(cache_path);

    uint64_t old_ts = sentry__usec_time() / 1000 - 2 * sentry__retry_backoff(4);
    sentry_uuid_t event_id = sentry_uuid_new_v4();
    write_retry_file(retry, old_ts, 4, &event_id);

    char uuid_str[37];
    sentry_uuid_as_string(&event_id, uuid_str);
    char cache_name[46];
    snprintf(cache_name, sizeof(cache_name), "%.36s.envelope", uuid_str);
    sentry_path_t *cached = sentry__path_join_str(cache_path, cache_name);

    TEST_CHECK_INT_EQUAL(count_envelope_files(cache_path), 1);
    TEST_CHECK(!sentry__path_is_file(cached));

    // Network error on a file at count=4 with max_retries=5 → renames to
    // cache format (<uuid>.envelope)
    retry_test_ctx_t ctx = { -1, 0 };
    sentry__retry_send(retry, 0, test_send_cb, &ctx);
    TEST_CHECK_INT_EQUAL(ctx.count, 1);
    TEST_CHECK_INT_EQUAL(count_envelope_files(cache_path), 1);
    TEST_CHECK(sentry__path_is_file(cached));

    // Success on a file at count=4 → also renames to cache format
    // (cache_keep preserves all envelopes regardless of send outcome)
    sentry__path_remove_all(cache_path);
    sentry__path_create_dir_all(cache_path);
    write_retry_file(retry, old_ts, 4, &event_id);
    TEST_CHECK(!sentry__path_is_file(cached));

    ctx = (retry_test_ctx_t) { 200, 0 };
    sentry__retry_send(retry, 0, test_send_cb, &ctx);
    TEST_CHECK_INT_EQUAL(ctx.count, 1);
    TEST_CHECK_INT_EQUAL(count_envelope_files(cache_path), 1);
    TEST_CHECK(sentry__path_is_file(cached));

    sentry__retry_free(retry);
    sentry__path_free(cached);
    sentry_close();
}

static int retry_func_calls = 0;

static void
mock_retry_func(void *state)
{
    (void)state;
    retry_func_calls++;
}

static void
noop_send(sentry_envelope_t *envelope, void *state)
{
    (void)state;
    sentry_envelope_free(envelope);
}

SENTRY_TEST(transport_retry)
{
    // no retry_func → no-op
    sentry_transport_t *transport = sentry_transport_new(noop_send);
    TEST_CHECK(!sentry__transport_can_retry(transport));
    sentry_transport_retry(transport);

    // with retry_func → calls it
    retry_func_calls = 0;
    sentry__transport_set_retry_func(transport, mock_retry_func);
    TEST_CHECK(sentry__transport_can_retry(transport));
    sentry_transport_retry(transport);
    TEST_CHECK_INT_EQUAL(retry_func_calls, 1);

    // NULL transport → no-op
    sentry_transport_retry(NULL);
    TEST_CHECK_INT_EQUAL(retry_func_calls, 1);

    sentry_transport_free(transport);
}

SENTRY_TEST(retry_backoff)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_http_retry(options, true);
    sentry_init(options);

    sentry_retry_t *retry = sentry__retry_new(options);
    TEST_ASSERT(!!retry);

    const sentry_path_t *cache_path = options->run->cache_path;
    sentry__path_remove_all(cache_path);
    sentry__path_create_dir_all(cache_path);

    uint64_t base = sentry__retry_backoff(0);
    uint64_t ref = sentry__usec_time() / 1000 - 10 * base;

    // retry 0: 10*base old, eligible (backoff=base)
    sentry_uuid_t id1 = sentry_uuid_new_v4();
    write_retry_file(retry, ref, 0, &id1);

    // retry 1: 1*base old, not yet eligible (backoff=2*base)
    sentry_uuid_t id2 = sentry_uuid_new_v4();
    write_retry_file(retry, ref + 9 * base, 1, &id2);

    // retry 1: 10*base old, eligible (backoff=2*base)
    sentry_uuid_t id3 = sentry_uuid_new_v4();
    write_retry_file(retry, ref, 1, &id3);

    // retry 2: 2*base old, not eligible (backoff=4*base)
    sentry_uuid_t id4 = sentry_uuid_new_v4();
    write_retry_file(retry, ref + 8 * base, 2, &id4);

    // With backoff: only eligible ones (id1 and id3) are sent
    retry_test_ctx_t ctx = { 200, 0 };
    sentry__retry_send(retry, 0, test_send_cb, &ctx);
    TEST_CHECK_INT_EQUAL(ctx.count, 2);
    TEST_CHECK_INT_EQUAL(count_envelope_files(cache_path), 2);

    // Startup scan (no backoff check): remaining 2 files are sent
    ctx = (retry_test_ctx_t) { 200, 0 };
    sentry__retry_send(retry, UINT64_MAX, test_send_cb, &ctx);
    TEST_CHECK_INT_EQUAL(ctx.count, 2);
    TEST_CHECK_INT_EQUAL(count_envelope_files(cache_path), 0);

    // Verify backoff calculation
    TEST_CHECK_UINT64_EQUAL(sentry__retry_backoff(0), base);
    TEST_CHECK_UINT64_EQUAL(sentry__retry_backoff(1), base * 2);
    TEST_CHECK_UINT64_EQUAL(sentry__retry_backoff(2), base * 4);
    TEST_CHECK_UINT64_EQUAL(sentry__retry_backoff(3), base * 8);
    TEST_CHECK_UINT64_EQUAL(sentry__retry_backoff(4), base * 16);
    TEST_CHECK_UINT64_EQUAL(sentry__retry_backoff(5), base * 32);
    TEST_CHECK_UINT64_EQUAL(sentry__retry_backoff(6), base * 32);
    TEST_CHECK_UINT64_EQUAL(sentry__retry_backoff(-1), base);

    sentry__retry_free(retry);
    sentry_close();
}

SENTRY_TEST(retry_trigger)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_http_retry(options, true);
    sentry_init(options);

    sentry_retry_t *retry = sentry__retry_new(options);
    TEST_ASSERT(!!retry);

    const sentry_path_t *cache_path = options->run->cache_path;
    sentry__path_remove_all(cache_path);
    sentry__path_create_dir_all(cache_path);

    uint64_t old_ts
        = sentry__usec_time() / 1000 - 10 * sentry__retry_backoff(0);
    sentry_uuid_t event_id = sentry_uuid_new_v4();
    write_retry_file(retry, old_ts, 0, &event_id);

    // UINT64_MAX (trigger mode) bypasses backoff: bumps count
    retry_test_ctx_t ctx = { -1, 0 };
    sentry__retry_send(retry, UINT64_MAX, test_send_cb, &ctx);
    TEST_CHECK_INT_EQUAL(ctx.count, 1);
    TEST_CHECK_INT_EQUAL(find_envelope_attempt(cache_path), 1);

    // second call: bumps again because UINT64_MAX skips backoff
    ctx = (retry_test_ctx_t) { -1, 0 };
    sentry__retry_send(retry, UINT64_MAX, test_send_cb, &ctx);
    TEST_CHECK_INT_EQUAL(ctx.count, 1);
    TEST_CHECK_INT_EQUAL(find_envelope_attempt(cache_path), 2);

    // before=0 (poll mode) respects backoff: item is skipped
    ctx = (retry_test_ctx_t) { -1, 0 };
    sentry__retry_send(retry, 0, test_send_cb, &ctx);
    TEST_CHECK_INT_EQUAL(ctx.count, 0);
    TEST_CHECK_INT_EQUAL(find_envelope_attempt(cache_path), 2);

    sentry__retry_free(retry);
    sentry_close();
}
