#include "sentry_testsupport.h"
#include "sentry_tracing.h"
#include "sentry_uuid.h"

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
    sentry_value_t tx_cxt = sentry_value_new_transaction(NULL, NULL);
    TEST_CHECK(!sentry_value_is_null(tx_cxt));
    const char *tx_name
        = sentry_value_as_string(sentry_value_get_by_key(tx_cxt, "name"));
    TEST_CHECK_STRING_EQUAL(tx_name, "<unlabeled transaction>");
    const char *tx_op
        = sentry_value_as_string(sentry_value_get_by_key(tx_cxt, "op"));
    TEST_CHECK_STRING_EQUAL(tx_op, "");
    TEST_CHECK(
        !sentry_value_is_null(sentry_value_get_by_key(tx_cxt, "trace_id")));
    TEST_CHECK(
        !sentry_value_is_null(sentry_value_get_by_key(tx_cxt, "span_id")));

    sentry_value_decref(tx_cxt);
    tx_cxt = sentry_value_new_transaction("", "");
    TEST_CHECK(!sentry_value_is_null(tx_cxt));
    tx_name = sentry_value_as_string(sentry_value_get_by_key(tx_cxt, "name"));
    TEST_CHECK_STRING_EQUAL(tx_name, "<unlabeled transaction>");
    TEST_CHECK_STRING_EQUAL(tx_op, "");
    TEST_CHECK(
        !sentry_value_is_null(sentry_value_get_by_key(tx_cxt, "trace_id")));
    TEST_CHECK(
        !sentry_value_is_null(sentry_value_get_by_key(tx_cxt, "span_id")));

    sentry_value_decref(tx_cxt);
    tx_cxt = sentry_value_new_transaction("honk.beep", "beepbeep");
    tx_name = sentry_value_as_string(sentry_value_get_by_key(tx_cxt, "name"));
    TEST_CHECK_STRING_EQUAL(tx_name, "honk.beep");
    tx_op = sentry_value_as_string(sentry_value_get_by_key(tx_cxt, "op"));
    TEST_CHECK_STRING_EQUAL(tx_op, "beepbeep");
    TEST_CHECK(
        !sentry_value_is_null(sentry_value_get_by_key(tx_cxt, "trace_id")));
    TEST_CHECK(
        !sentry_value_is_null(sentry_value_get_by_key(tx_cxt, "span_id")));

    sentry_transaction_set_name(tx_cxt, "");
    tx_name = sentry_value_as_string(sentry_value_get_by_key(tx_cxt, "name"));
    TEST_CHECK_STRING_EQUAL(tx_name, "<unlabeled transaction>");

    sentry_transaction_set_operation(tx_cxt, "");
    tx_op = sentry_value_as_string(sentry_value_get_by_key(tx_cxt, "op"));
    TEST_CHECK_STRING_EQUAL(tx_op, "");

    sentry_transaction_set_sampled(tx_cxt, 1);
    TEST_CHECK(
        sentry_value_is_true(sentry_value_get_by_key(tx_cxt, "sampled")) == 1);

    sentry_value_decref(tx_cxt);
}

static void
send_transaction_envelope_test_basic(sentry_envelope_t *envelope, void *data)
{
    uint64_t *called = data;
    *called += 1;

    sentry_value_t transaction = sentry_envelope_get_transaction(envelope);
    TEST_CHECK(!sentry_value_is_null(transaction));
    const char *event_id = sentry_value_as_string(
        sentry_value_get_by_key(transaction, "event_id"));
    TEST_CHECK_STRING_EQUAL(event_id, "4c035723-8638-4c3a-923f-2ab9d08b4018");

    if (*called == 1) {
        const char *type = sentry_value_as_string(
            sentry_value_get_by_key(transaction, "type"));
        TEST_CHECK_STRING_EQUAL(type, "transaction");
        const char *name = sentry_value_as_string(
            sentry_value_get_by_key(transaction, "transaction"));
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

    sentry_value_t transaction
        = sentry_value_new_transaction("How could you", "Don't capture this.");
    transaction = sentry_transaction_start(transaction);
    sentry_transaction_finish(transaction);
    sentry_user_consent_give();

    transaction = sentry_value_new_transaction("honk", "beep");
    transaction = sentry_transaction_start(transaction);
    sentry_transaction_finish(transaction);

    sentry_user_consent_revoke();
    transaction = sentry_value_new_transaction(
        "How could you again", "Don't capture this either.");
    transaction = sentry_transaction_start(transaction);
    sentry_transaction_finish(transaction);

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

    for (int i = 0; i < 100; i++) {
        sentry_value_t transaction
            = sentry_value_new_transaction("honk", "beep");
        transaction = sentry_transaction_start(transaction);
        sentry_transaction_finish(transaction);
    }

    sentry_close();

    // well, its random after all
    TEST_CHECK(called_transport > 50 && called_transport < 100);
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

    sentry_value_t transaction = sentry_value_new_transaction("honk", "beep");
    transaction = sentry_transaction_start(transaction);
    sentry_transaction_finish(transaction);

    sentry_close();

    TEST_CHECK_INT_EQUAL(called_transport, 1);
    TEST_CHECK_INT_EQUAL(called_beforesend, 0);
}
