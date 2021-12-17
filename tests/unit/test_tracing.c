#include "sentry_testsupport.h"
#include "sentry_tracing.h"
#include "sentry_uuid.h"

#define IS_NULL(Src, Field)                                                    \
    sentry_value_is_null(sentry_value_get_by_key(Src, Field))
#define CHECK_STRING_PROPERTY(Src, Field, Expected)                            \
    TEST_CHECK_STRING_EQUAL(                                                   \
        sentry_value_as_string(sentry_value_get_by_key(Src, Field)), Expected)

SENTRY_TEST(basic_tracing_context)
{
    sentry_value_t span = sentry_value_new_object();
    TEST_CHECK(sentry_value_is_null(sentry__span_get_trace_context(span)));

    sentry_value_set_by_key(span, "op", sentry_value_new_string("honk.beep"));
    TEST_CHECK(sentry_value_is_null(sentry__span_get_trace_context(span)));

    sentry_uuid_t trace_id = sentry_uuid_new_v4();
    sentry_value_set_by_key(
        span, "trace_id", sentry__value_new_internal_uuid(&trace_id));
    TEST_CHECK(sentry_value_is_null(sentry__span_get_trace_context(span)));

    sentry_uuid_t span_id = sentry_uuid_new_v4();
    sentry_value_set_by_key(
        span, "span_id", sentry__value_new_span_uuid(&span_id));

    sentry_value_t trace_context = sentry__span_get_trace_context(span);
    TEST_CHECK(!sentry_value_is_null(trace_context));
    TEST_CHECK(!sentry_value_is_null(
        sentry_value_get_by_key(trace_context, "trace_id")));
    TEST_CHECK(!sentry_value_is_null(
        sentry_value_get_by_key(trace_context, "span_id")));

    const char *span_op
        = sentry_value_as_string(sentry_value_get_by_key(trace_context, "op"));
    TEST_CHECK_STRING_EQUAL(span_op, "honk.beep");

    sentry_value_decref(trace_context);
    sentry_value_decref(span);
}

SENTRY_TEST(basic_transaction)
{
    sentry_value_t tx_cxt = sentry_value_new_transaction_context(NULL, NULL);
    TEST_CHECK(!sentry_value_is_null(tx_cxt));
    CHECK_STRING_PROPERTY(tx_cxt, "transaction", "");
    CHECK_STRING_PROPERTY(tx_cxt, "op", "");
    TEST_CHECK(!IS_NULL(tx_cxt, "trace_id"));
    TEST_CHECK(!IS_NULL(tx_cxt, "span_id"));

    sentry_value_decref(tx_cxt);
    tx_cxt = sentry_value_new_transaction_context("", "");
    TEST_CHECK(!sentry_value_is_null(tx_cxt));
    CHECK_STRING_PROPERTY(tx_cxt, "transaction", "");
    CHECK_STRING_PROPERTY(tx_cxt, "op", "");
    TEST_CHECK(!IS_NULL(tx_cxt, "trace_id"));
    TEST_CHECK(!IS_NULL(tx_cxt, "span_id"));

    sentry_value_decref(tx_cxt);
    tx_cxt = sentry_value_new_transaction_context("honk.beep", "beepbeep");
    CHECK_STRING_PROPERTY(tx_cxt, "transaction", "honk.beep");
    CHECK_STRING_PROPERTY(tx_cxt, "op", "beepbeep");
    TEST_CHECK(!IS_NULL(tx_cxt, "trace_id"));
    TEST_CHECK(!IS_NULL(tx_cxt, "span_id"));

    sentry_transaction_context_set_name(tx_cxt, "");
    CHECK_STRING_PROPERTY(tx_cxt, "transaction", "");

    sentry_transaction_context_set_operation(tx_cxt, "");
    CHECK_STRING_PROPERTY(tx_cxt, "op", "");

    sentry_transaction_context_set_sampled(tx_cxt, 1);
    TEST_CHECK(
        sentry_value_is_true(sentry_value_get_by_key(tx_cxt, "sampled")) == 1);

    sentry_value_decref(tx_cxt);
}

static void
check_backfilled_name(sentry_envelope_t *envelope, void *data)
{
    uint64_t *called = data;
    *called += 1;

    sentry_value_t tx = sentry_envelope_get_transaction(envelope);
    TEST_CHECK(!sentry_value_is_null(tx));
    CHECK_STRING_PROPERTY(tx, "transaction", "<unlabeled transaction>");

    sentry_envelope_free(envelope);
}

SENTRY_TEST(transaction_name_backfill_on_finish)
{
    uint64_t called = 0;

    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");

    sentry_transport_t *transport = sentry_transport_new(check_backfilled_name);
    sentry_transport_set_state(transport, &called);
    sentry_options_set_transport(options, transport);

    sentry_options_set_traces_sample_rate(options, 1.0);
    sentry_init(options);

    sentry_value_t tx_cxt = sentry_value_new_transaction_context(NULL, NULL);
    sentry_value_t tx = sentry_transaction_start(tx_cxt);
    sentry_uuid_t event_id = sentry_transaction_finish(tx);
    TEST_CHECK(!sentry_uuid_is_nil(&event_id));

    tx_cxt = sentry_value_new_transaction_context("", "");
    tx = sentry_transaction_start(tx_cxt);
    event_id = sentry_transaction_finish(tx);
    TEST_CHECK(!sentry_uuid_is_nil(&event_id));

    sentry_close();
    TEST_CHECK_INT_EQUAL(called, 2);
}

static void
send_transaction_envelope_test_basic(sentry_envelope_t *envelope, void *data)
{
    uint64_t *called = data;
    *called += 1;

    sentry_value_t tx = sentry_envelope_get_transaction(envelope);
    TEST_CHECK(!sentry_value_is_null(tx));
    const char *event_id
        = sentry_value_as_string(sentry_value_get_by_key(tx, "event_id"));
    TEST_CHECK_STRING_EQUAL(event_id, "4c035723-8638-4c3a-923f-2ab9d08b4018");

    if (*called == 1) {
        const char *type
            = sentry_value_as_string(sentry_value_get_by_key(tx, "type"));
        TEST_CHECK_STRING_EQUAL(type, "transaction");
        const char *name = sentry_value_as_string(
            sentry_value_get_by_key(tx, "transaction"));
        TEST_CHECK_STRING_EQUAL(name, "honk");
    }

    sentry_envelope_free(envelope);
}

SENTRY_TEST(basic_function_transport_transaction)
{
    uint64_t called = 0;

    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");

    sentry_transport_t *transport
        = sentry_transport_new(send_transaction_envelope_test_basic);
    sentry_transport_set_state(transport, &called);
    sentry_options_set_transport(options, transport);

    sentry_options_set_traces_sample_rate(options, 1.0);
    sentry_options_set_require_user_consent(options, true);
    sentry_init(options);

    sentry_value_t tx_cxt = sentry_value_new_transaction_context(
        "How could you", "Don't capture this.");
    sentry_value_t tx = sentry_transaction_start(tx_cxt);
    sentry_uuid_t event_id = sentry_transaction_finish(tx);
    // TODO: `sentry_capture_event` acts as if the event was sent if user
    // consent was not given
    TEST_CHECK(!sentry_uuid_is_nil(&event_id));
    sentry_user_consent_give();

    tx_cxt = sentry_value_new_transaction_context("honk", "beep");
    tx = sentry_transaction_start(tx_cxt);
    event_id = sentry_transaction_finish(tx);
    TEST_CHECK(!sentry_uuid_is_nil(&event_id));

    sentry_user_consent_revoke();
    tx_cxt = sentry_value_new_transaction_context(
        "How could you again", "Don't capture this either.");
    tx = sentry_transaction_start(tx_cxt);
    event_id = sentry_transaction_finish(tx);
    // TODO: `sentry_capture_event` acts as if the event was sent if user
    // consent was not given
    TEST_CHECK(!sentry_uuid_is_nil(&event_id));

    sentry_close();

    TEST_CHECK_INT_EQUAL(called, 1);
}

SENTRY_TEST(transport_sampling_transactions)
{
    uint64_t called_transport = 0;

    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");

    sentry_transport_t *transport
        = sentry_transport_new(send_transaction_envelope_test_basic);
    sentry_transport_set_state(transport, &called_transport);
    sentry_options_set_transport(options, transport);

    sentry_options_set_traces_sample_rate(options, 0.75);
    sentry_init(options);

    uint64_t sent_transactions = 0;
    for (int i = 0; i < 100; i++) {
        sentry_value_t tx_cxt
            = sentry_value_new_transaction_context("honk", "beep");
        sentry_value_t tx = sentry_transaction_start(tx_cxt);
        sentry_uuid_t event_id = sentry_transaction_finish(tx);
        if (!sentry_uuid_is_nil(&event_id)) {
            sent_transactions += 1;
        }
    }

    sentry_close();

    // well, its random after all
    TEST_CHECK(called_transport > 50 && called_transport < 100);
    TEST_CHECK(called_transport == sent_transactions);
}

static sentry_value_t
before_send(sentry_value_t event, void *UNUSED(hint), void *data)
{
    uint64_t *called = data;
    *called += 1;

    sentry_value_decref(event);
    return sentry_value_new_null();
}

SENTRY_TEST(transactions_skip_before_send)
{
    uint64_t called_beforesend = 0;
    uint64_t called_transport = 0;

    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");

    sentry_transport_t *transport
        = sentry_transport_new(send_transaction_envelope_test_basic);
    sentry_transport_set_state(transport, &called_transport);
    sentry_options_set_transport(options, transport);

    sentry_options_set_traces_sample_rate(options, 1.0);
    sentry_options_set_before_send(options, before_send, &called_beforesend);
    sentry_init(options);

    sentry_value_t tx_cxt
        = sentry_value_new_transaction_context("honk", "beep");
    sentry_value_t tx = sentry_transaction_start(tx_cxt);
    sentry_uuid_t event_id = sentry_transaction_finish(tx);
    TEST_CHECK(!sentry_uuid_is_nil(&event_id));

    sentry_close();

    TEST_CHECK_INT_EQUAL(called_transport, 1);
    TEST_CHECK_INT_EQUAL(called_beforesend, 0);
}

#undef IS_NULL
#undef CHECK_STRING_PROPERTY
