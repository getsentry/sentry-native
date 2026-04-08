#include "sentry_batcher.h"
#include "sentry_client_report.h"
#include "sentry_envelope.h"
#include "sentry_path.h"
#include "sentry_ratelimiter.h"
#include "sentry_sync.h"
#include "sentry_testsupport.h"
#include "sentry_value.h"

SENTRY_TEST(client_report_discard)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    TEST_CHECK(!sentry__client_report_has_pending());

    sentry__client_report_discard(
        SENTRY_DISCARD_REASON_SAMPLE_RATE, SENTRY_DATA_CATEGORY_ERROR, 2);
    sentry__client_report_discard(
        SENTRY_DISCARD_REASON_BEFORE_SEND, SENTRY_DATA_CATEGORY_TRANSACTION, 1);

    TEST_CHECK(sentry__client_report_has_pending());

    sentry_envelope_t *envelope = sentry__envelope_new();
    TEST_CHECK(!!envelope);

    sentry_envelope_item_t *item
        = sentry__client_report_into_envelope(envelope);
    TEST_CHECK(!!item);

    TEST_CHECK(!sentry__client_report_has_pending());

    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry__envelope_item_get_header(item, "type")),
        "client_report");

    size_t payload_len = 0;
    const char *payload = sentry__envelope_item_get_payload(item, &payload_len);
    TEST_CHECK(!!payload);
    TEST_CHECK(payload_len > 0);

    sentry_value_t report = sentry__value_from_json(payload, payload_len);
    TEST_CHECK(!sentry_value_is_null(report));

    TEST_CHECK(
        !sentry_value_is_null(sentry_value_get_by_key(report, "timestamp")));

    sentry_value_t discarded
        = sentry_value_get_by_key(report, "discarded_events");
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(discarded), 2);

    sentry_value_t entry0 = sentry_value_get_by_index(discarded, 0);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry0, "reason")),
        "sample_rate");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry0, "category")),
        "error");
    TEST_CHECK_INT_EQUAL(
        sentry_value_as_int32(sentry_value_get_by_key(entry0, "quantity")), 2);

    sentry_value_t entry1 = sentry_value_get_by_index(discarded, 1);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry1, "reason")),
        "before_send");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry1, "category")),
        "transaction");
    TEST_CHECK_INT_EQUAL(
        sentry_value_as_int32(sentry_value_get_by_key(entry1, "quantity")), 1);

    sentry_value_decref(report);
    sentry_envelope_free(envelope);
    sentry_close();
}

SENTRY_TEST(client_report_discard_envelope)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    TEST_CHECK(!sentry__client_report_has_pending());

    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry__envelope_add_from_buffer(envelope, "{}", 2, "event");
    sentry__envelope_add_from_buffer(envelope, "{}", 2, "session");
    sentry__envelope_add_from_buffer(envelope, "{}", 2, "transaction");
    sentry__envelope_add_from_buffer(envelope, "{}", 2, "attachment");
    sentry__envelope_add_from_buffer(envelope, "{}", 2, "log");
    sentry__envelope_add_from_buffer(envelope, "{}", 2, "feedback");
    sentry__envelope_add_from_buffer(envelope, "{}", 2, "trace_metric");
    sentry__envelope_add_from_buffer(envelope, "{}", 2, "client_report");

    sentry__client_report_discard_envelope(
        envelope, SENTRY_DISCARD_REASON_NETWORK_ERROR, NULL);

    TEST_CHECK(sentry__client_report_has_pending());

    sentry_envelope_t *carrier = sentry__envelope_new();
    sentry_envelope_item_t *item = sentry__client_report_into_envelope(carrier);
    TEST_CHECK(!!item);

    size_t payload_len = 0;
    const char *payload = sentry__envelope_item_get_payload(item, &payload_len);
    sentry_value_t report = sentry__value_from_json(payload, payload_len);

    sentry_value_t discarded
        = sentry_value_get_by_key(report, "discarded_events");
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(discarded), 7);

    sentry_value_t entry0 = sentry_value_get_by_index(discarded, 0);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry0, "reason")),
        "network_error");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry0, "category")),
        "error");
    TEST_CHECK_INT_EQUAL(
        sentry_value_as_int32(sentry_value_get_by_key(entry0, "quantity")), 1);

    sentry_value_t entry1 = sentry_value_get_by_index(discarded, 1);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry1, "reason")),
        "network_error");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry1, "category")),
        "session");
    TEST_CHECK_INT_EQUAL(
        sentry_value_as_int32(sentry_value_get_by_key(entry1, "quantity")), 1);

    sentry_value_t entry2 = sentry_value_get_by_index(discarded, 2);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry2, "reason")),
        "network_error");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry2, "category")),
        "transaction");
    TEST_CHECK_INT_EQUAL(
        sentry_value_as_int32(sentry_value_get_by_key(entry2, "quantity")), 1);

    sentry_value_t entry3 = sentry_value_get_by_index(discarded, 3);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry3, "reason")),
        "network_error");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry3, "category")),
        "attachment");
    TEST_CHECK_INT_EQUAL(
        sentry_value_as_int32(sentry_value_get_by_key(entry3, "quantity")), 1);

    sentry_value_t entry4 = sentry_value_get_by_index(discarded, 4);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry4, "reason")),
        "network_error");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry4, "category")),
        "log_item");
    TEST_CHECK_INT_EQUAL(
        sentry_value_as_int32(sentry_value_get_by_key(entry4, "quantity")), 1);

    sentry_value_t entry5 = sentry_value_get_by_index(discarded, 5);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry5, "reason")),
        "network_error");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry5, "category")),
        "feedback");
    TEST_CHECK_INT_EQUAL(
        sentry_value_as_int32(sentry_value_get_by_key(entry5, "quantity")), 1);

    sentry_value_t entry6 = sentry_value_get_by_index(discarded, 6);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry6, "reason")),
        "network_error");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry6, "category")),
        "trace_metric");
    TEST_CHECK_INT_EQUAL(
        sentry_value_as_int32(sentry_value_get_by_key(entry6, "quantity")), 1);

    sentry_value_decref(report);
    sentry_envelope_free(carrier);
    sentry_envelope_free(envelope);
    sentry_close();
}

SENTRY_TEST(client_report_discard_rate_limited)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry__envelope_add_from_buffer(envelope, "{}", 2, "event");
    sentry__envelope_add_from_buffer(envelope, "{}", 2, "session");

    // Rate-limit the error category
    sentry_rate_limiter_t *rl = sentry__rate_limiter_new();
    sentry__rate_limiter_update_from_header(rl, "60:error:organization");

    // Discard with RL: should only record session, not event
    sentry__client_report_discard_envelope(
        envelope, SENTRY_DISCARD_REASON_NETWORK_ERROR, rl);

    sentry_envelope_t *carrier = sentry__envelope_new();
    sentry_envelope_item_t *item = sentry__client_report_into_envelope(carrier);
    TEST_CHECK(!!item);

    size_t payload_len = 0;
    const char *payload = sentry__envelope_item_get_payload(item, &payload_len);
    sentry_value_t report = sentry__value_from_json(payload, payload_len);

    sentry_value_t discarded
        = sentry_value_get_by_key(report, "discarded_events");
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(discarded), 1);

    sentry_value_t entry0 = sentry_value_get_by_index(discarded, 0);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry0, "reason")),
        "network_error");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry0, "category")),
        "session");
    TEST_CHECK_INT_EQUAL(
        sentry_value_as_int32(sentry_value_get_by_key(entry0, "quantity")), 1);

    sentry_value_decref(report);
    sentry_envelope_free(carrier);
    sentry_envelope_free(envelope);
    sentry__rate_limiter_free(rl);
    sentry_close();
}

SENTRY_TEST(client_report_none)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    TEST_CHECK(!sentry__client_report_has_pending());

    sentry_envelope_t *envelope = sentry__envelope_new();
    TEST_CHECK(!!envelope);

    sentry_envelope_item_t *item
        = sentry__client_report_into_envelope(envelope);
    TEST_CHECK(!item);

    sentry_envelope_free(envelope);
    sentry_close();
}

static sentry_envelope_item_t *
dummy_batch_func(sentry_envelope_t *envelope, sentry_value_t items)
{
    (void)envelope;
    sentry_value_decref(items);
    return NULL;
}

SENTRY_TEST(client_report_queue_overflow)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    sentry_batcher_t *batcher
        = sentry__batcher_new(dummy_batch_func, SENTRY_DATA_CATEGORY_LOG_ITEM);
    TEST_CHECK(!!batcher);

    // Fill the buffer (SENTRY_BATCHER_QUEUE_LENGTH is 5 in unit tests)
    for (int i = 0; i < SENTRY_BATCHER_QUEUE_LENGTH; i++) {
        TEST_CHECK(sentry__batcher_enqueue(batcher, sentry_value_new_null()));
    }

    TEST_CHECK(!sentry__client_report_has_pending());

    // This should overflow and record a discard
    TEST_CHECK(!sentry__batcher_enqueue(batcher, sentry_value_new_null()));

    TEST_CHECK(sentry__client_report_has_pending());

    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry_envelope_item_t *cr_item
        = sentry__client_report_into_envelope(envelope);
    TEST_CHECK(!!cr_item);

    size_t payload_len = 0;
    const char *payload
        = sentry__envelope_item_get_payload(cr_item, &payload_len);
    sentry_value_t report = sentry__value_from_json(payload, payload_len);

    sentry_value_t discarded
        = sentry_value_get_by_key(report, "discarded_events");
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(discarded), 1);

    sentry_value_t entry0 = sentry_value_get_by_index(discarded, 0);
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry0, "reason")),
        "queue_overflow");
    TEST_CHECK_STRING_EQUAL(
        sentry_value_as_string(sentry_value_get_by_key(entry0, "category")),
        "log_item");
    TEST_CHECK_INT_EQUAL(
        sentry_value_as_int32(sentry_value_get_by_key(entry0, "quantity")), 1);

    sentry_value_decref(report);
    sentry_envelope_free(envelope);
    sentry__batcher_release(batcher);
    sentry_close();
}

#define DISCARD_PER_THREAD 10000
#define NUM_THREADS 8

static volatile long g_produced = 0;
static volatile long g_consumed = 0;
static volatile long g_running = 1;

SENTRY_THREAD_FN
discard_thread_func(void *data)
{
    (void)data;
    for (int i = 0; i < DISCARD_PER_THREAD; i++) {
        sentry__client_report_discard(
            SENTRY_DISCARD_REASON_SAMPLE_RATE, SENTRY_DATA_CATEGORY_ERROR, 1);
        sentry__atomic_fetch_and_add((long *)&g_produced, 1);
    }
    return 0;
}

SENTRY_THREAD_FN
flush_thread_func(void *data)
{
    (void)data;
    while (sentry__atomic_fetch((long *)&g_running)) {
        sentry_envelope_t *envelope = sentry__envelope_new();
        sentry_envelope_item_t *item
            = sentry__client_report_into_envelope(envelope);
        if (item) {
            size_t payload_len = 0;
            const char *payload
                = sentry__envelope_item_get_payload(item, &payload_len);
            sentry_value_t report
                = sentry__value_from_json(payload, payload_len);
            sentry_value_t discarded
                = sentry_value_get_by_key(report, "discarded_events");
            for (uint32_t j = 0; j < sentry_value_get_length(discarded); j++) {
                sentry_value_t entry = sentry_value_get_by_index(discarded, j);
                sentry__atomic_fetch_and_add((long *)&g_consumed,
                    sentry_value_as_int32(
                        sentry_value_get_by_key(entry, "quantity")));
            }
            sentry_value_decref(report);
        }
        sentry_envelope_free(envelope);
    }
    return 0;
}

SENTRY_TEST(client_report_concurrent)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_init(options);

    sentry__atomic_store((long *)&g_produced, 0);
    sentry__atomic_store((long *)&g_consumed, 0);
    sentry__atomic_store((long *)&g_running, 1);

    sentry_threadid_t discard_threads[NUM_THREADS];
    sentry_threadid_t flush_threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        sentry__thread_init(&discard_threads[i]);
        sentry__thread_spawn(&discard_threads[i], discard_thread_func, NULL);
        sentry__thread_init(&flush_threads[i]);
        sentry__thread_spawn(&flush_threads[i], flush_thread_func, NULL);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        sentry__thread_join(discard_threads[i]);
    }
    sentry__atomic_store((long *)&g_running, 0);
    for (int i = 0; i < NUM_THREADS; i++) {
        sentry__thread_join(flush_threads[i]);
    }

    // drain remaining
    sentry_envelope_t *envelope = sentry__envelope_new();
    sentry_envelope_item_t *item
        = sentry__client_report_into_envelope(envelope);
    if (item) {
        size_t payload_len = 0;
        const char *payload
            = sentry__envelope_item_get_payload(item, &payload_len);
        sentry_value_t report = sentry__value_from_json(payload, payload_len);
        sentry_value_t discarded
            = sentry_value_get_by_key(report, "discarded_events");
        for (uint32_t j = 0; j < sentry_value_get_length(discarded); j++) {
            sentry_value_t entry = sentry_value_get_by_index(discarded, j);
            sentry__atomic_fetch_and_add((long *)&g_consumed,
                sentry_value_as_int32(
                    sentry_value_get_by_key(entry, "quantity")));
        }
        sentry_value_decref(report);
    }
    sentry_envelope_free(envelope);

    TEST_CHECK(!sentry__client_report_has_pending());
    TEST_CHECK_INT_EQUAL(sentry__atomic_fetch((long *)&g_consumed),
        NUM_THREADS * DISCARD_PER_THREAD);

    sentry_close();
}
