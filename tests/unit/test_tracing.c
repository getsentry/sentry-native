#include "sentry_scope.h"
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
    TEST_CHECK(!sentry_value_is_null(tx_cxt));
    tx_cxt = sentry_value_new_transaction_context("", "");
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

    // exact value is nondeterministic because of rng
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

static void
before_transport(sentry_envelope_t *envelope, void *data)
{
    uint64_t *called = data;
    *called += 1;

    sentry_envelope_free(envelope);
}

SENTRY_TEST(multiple_transactions)
{
    uint64_t called_transport = 0;

    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");

    sentry_transport_t *transport = sentry_transport_new(before_transport);
    sentry_transport_set_state(transport, &called_transport);
    sentry_options_set_transport(options, transport);

    sentry_options_set_traces_sample_rate(options, 1.0);
    sentry_init(options);

    sentry_value_t tx_cxt = sentry_value_new_transaction_context("wow!", NULL);
    sentry_value_t tx = sentry_transaction_start(tx_cxt);
    sentry_set_span(tx);

    sentry_value_t scope_tx = sentry__scope_get_span();
    CHECK_STRING_PROPERTY(scope_tx, "transaction", "wow!");

    sentry_uuid_t event_id = sentry_transaction_finish(tx);
    scope_tx = sentry__scope_get_span();
    TEST_CHECK(sentry_value_is_null(scope_tx));
    TEST_CHECK(!sentry_uuid_is_nil(&event_id));

    // Set transaction on scope twice, back-to-back without finishing the first
    // one
    tx_cxt = sentry_value_new_transaction_context("whoa!", NULL);
    tx = sentry_transaction_start(tx_cxt);
    sentry_set_span(tx);
    sentry_value_decref(tx);
    tx_cxt = sentry_value_new_transaction_context("wowee!", NULL);
    tx = sentry_transaction_start(tx_cxt);
    sentry_set_span(tx);
    scope_tx = sentry__scope_get_span();
    CHECK_STRING_PROPERTY(scope_tx, "transaction", "wowee!");
    event_id = sentry_transaction_finish(tx);
    TEST_CHECK(!sentry_uuid_is_nil(&event_id));

    sentry_close();

    TEST_CHECK_INT_EQUAL(called_transport, 2);
}

SENTRY_TEST(basic_spans)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_traces_sample_rate(options, 1.0);
    sentry_options_set_max_spans(options, 3);
    sentry_init(options);

    sentry_value_t tx_cxt = sentry_value_new_transaction_context("wow!", NULL);
    sentry_value_t tx = sentry_transaction_start(tx_cxt);

    sentry_value_t child = sentry_span_start_child(tx, "honk", "goose");
    TEST_CHECK(!sentry_value_is_null(child));

    // Peek into the transaction's span list and make sure everything is
    // good
    const char *trace_id
        = sentry_value_as_string(sentry_value_get_by_key(tx, "trace_id"));
    const char *parent_span_id
        = sentry_value_as_string(sentry_value_get_by_key(tx, "span_id"));
    // Don't track the span yet
    TEST_CHECK(IS_NULL(tx, "spans"));

    sentry_span_finish(tx, child);

    TEST_CHECK(!IS_NULL(tx, "spans"));
    sentry_value_t spans = sentry_value_get_by_key(tx, "spans");
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(spans), 1);

    sentry_value_t stored_child = sentry_value_get_by_index(spans, 0);
    // Make sure the span inherited everything correctly
    CHECK_STRING_PROPERTY(stored_child, "trace_id", trace_id);
    CHECK_STRING_PROPERTY(stored_child, "parent_span_id", parent_span_id);
    CHECK_STRING_PROPERTY(stored_child, "op", "honk");
    CHECK_STRING_PROPERTY(stored_child, "description", "goose");
    // Should be finished
    TEST_CHECK(!IS_NULL(stored_child, "timestamp"));

    sentry_value_decref(tx);

    sentry_close();
}

SENTRY_TEST(spans_on_scope)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_traces_sample_rate(options, 1.0);
    sentry_options_set_max_spans(options, 3);
    sentry_init(options);

    sentry_value_t tx_cxt = sentry_value_new_transaction_context("wow!", NULL);
    sentry_value_t tx = sentry_transaction_start(tx_cxt);
    tx = sentry_set_span(tx);

    sentry_value_t child = sentry_span_start_child(tx, "honk", "goose");
    TEST_CHECK(!sentry_value_is_null(child));

    // Peek into the transaction's span list and make sure everything is
    // good
    sentry_value_t scope_tx = sentry__scope_get_span();
    const char *trace_id
        = sentry_value_as_string(sentry_value_get_by_key(scope_tx, "trace_id"));
    const char *parent_span_id
        = sentry_value_as_string(sentry_value_get_by_key(scope_tx, "span_id"));
    // Don't track the span yet
    TEST_CHECK(IS_NULL(scope_tx, "spans"));

    sentry_span_finish(tx, child);

    scope_tx = sentry__scope_get_span();
    TEST_CHECK(!IS_NULL(scope_tx, "spans"));
    sentry_value_t spans = sentry_value_get_by_key(scope_tx, "spans");
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(spans), 1);

    sentry_value_t stored_child = sentry_value_get_by_index(spans, 0);
    // Make sure the span inherited everything correctly
    CHECK_STRING_PROPERTY(stored_child, "trace_id", trace_id);
    CHECK_STRING_PROPERTY(stored_child, "parent_span_id", parent_span_id);
    CHECK_STRING_PROPERTY(stored_child, "op", "honk");
    CHECK_STRING_PROPERTY(stored_child, "description", "goose");
    // Should be finished
    TEST_CHECK(!IS_NULL(stored_child, "timestamp"));

    sentry_value_decref(tx);

    sentry_close();
}

SENTRY_TEST(child_spans)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_traces_sample_rate(options, 1.0);
    sentry_options_set_max_spans(options, 3);
    sentry_init(options);

    // Finishing a nonexistent span doesn't explode anything
    sentry_value_t fake_span
        = sentry__value_new_span(sentry_value_new_null(), NULL);
    sentry_value_t fake_tx = sentry_value_new_null();
    sentry_span_finish(fake_tx, fake_span);

    sentry_value_t tx_cxt = sentry_value_new_transaction_context("wow!", NULL);
    sentry_value_t tx = sentry_transaction_start(tx_cxt);

    sentry_value_t child = sentry_span_start_child(tx, "honk", "goose");
    TEST_CHECK(!sentry_value_is_null(child));
    // Shouldn't be added to spans yet
    TEST_CHECK(IS_NULL(tx, "spans"));

    sentry_value_t grandchild = sentry_span_start_child(child, "beep", "car");
    TEST_CHECK(!sentry_value_is_null(grandchild));
    // Shouldn't be added to spans yet
    TEST_CHECK(IS_NULL(tx, "spans"));

    sentry_span_finish(tx, grandchild);

    // Make sure everything on the transaction looks good, check grandchild
    const char *trace_id
        = sentry_value_as_string(sentry_value_get_by_key(tx, "trace_id"));
    const char *parent_span_id
        = sentry_value_as_string(sentry_value_get_by_key(child, "span_id"));

    TEST_CHECK(!IS_NULL(tx, "spans"));
    sentry_value_t spans = sentry_value_get_by_key(tx, "spans");
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(spans), 1);

    sentry_value_t stored_grandchild = sentry_value_get_by_index(spans, 0);
    CHECK_STRING_PROPERTY(stored_grandchild, "trace_id", trace_id);
    CHECK_STRING_PROPERTY(stored_grandchild, "parent_span_id", parent_span_id);
    CHECK_STRING_PROPERTY(stored_grandchild, "op", "beep");
    CHECK_STRING_PROPERTY(stored_grandchild, "description", "car");
    // Should be finished
    TEST_CHECK(!IS_NULL(stored_grandchild, "timestamp"));

    sentry_span_finish(tx, child);
    spans = sentry_value_get_by_key(tx, "spans");
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(spans), 2);

    sentry_value_decref(tx);

    sentry_close();
}

SENTRY_TEST(overflow_spans)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_traces_sample_rate(options, 1.0);
    sentry_options_set_max_spans(options, 1);
    sentry_init(options);

    sentry_value_t tx_cxt = sentry_value_new_transaction_context("wow!", NULL);
    sentry_value_t tx = sentry_transaction_start(tx_cxt);

    sentry_value_t child = sentry_span_start_child(tx, "honk", "goose");
    const char *child_span_id
        = sentry_value_as_string(sentry_value_get_by_key(child, "span_id"));

    // Shouldn't be added to spans yet
    TEST_CHECK(IS_NULL(tx, "spans"));

    sentry_value_t overflow_child
        = sentry_span_start_child(child, "beep", "car");
    TEST_CHECK(!sentry_value_is_null(overflow_child));
    // Shouldn't be added to spans yet
    TEST_CHECK(IS_NULL(tx, "spans"));

    sentry_span_finish(tx, child);

    TEST_CHECK(!IS_NULL(tx, "spans"));
    sentry_value_t spans = sentry_value_get_by_key(tx, "spans");
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(spans), 1);

    sentry_value_t stored_child = sentry_value_get_by_index(spans, 0);
    CHECK_STRING_PROPERTY(stored_child, "span_id", child_span_id);

    sentry_value_t second_overflow_child
        = sentry_span_start_child(child, "ring", "bicycle");
    TEST_CHECK(sentry_value_is_null(second_overflow_child));

    sentry_span_finish(tx, overflow_child);
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(spans), 1);

    sentry_value_decref(tx);

    sentry_close();
}

SENTRY_TEST(wrong_spans_on_transaction_is_ok)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_traces_sample_rate(options, 1.0);
    sentry_options_set_max_spans(options, 5);
    sentry_init(options);

    sentry_value_t tx_cxt = sentry_value_new_transaction_context("wow!", NULL);
    sentry_value_t tx = sentry_transaction_start(tx_cxt);

    sentry_value_t child = sentry_span_start_child(tx, "honk", "goose");
    const char *child_span_id
        = sentry_value_as_string(sentry_value_get_by_key(child, "span_id"));

    sentry_value_t lingering_child = sentry_span_start_child(tx, "beep", "car");

    sentry_value_t tx_cxt_other
        = sentry_value_new_transaction_context("whoa!", NULL);
    sentry_value_t tx_other = sentry_transaction_start(tx_cxt_other);

    sentry_span_finish(tx_other, child);

    // doesn't care if the child has been finished on the wrong transaction
    TEST_CHECK(IS_NULL(tx, "spans"));
    TEST_CHECK(!IS_NULL(tx_other, "spans"));

    sentry_value_t spans = sentry_value_get_by_key(tx_other, "spans");
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(spans), 1);

    sentry_value_t stored_child = sentry_value_get_by_index(spans, 0);
    CHECK_STRING_PROPERTY(stored_child, "span_id", child_span_id);

    sentry_transaction_finish(tx);

    // doesn't care if the child belonged to a different, already finished
    // transaction
    sentry_span_finish(tx_other, lingering_child);
    TEST_CHECK(!IS_NULL(tx_other, "spans"));
    spans = sentry_value_get_by_key(tx_other, "spans");
    TEST_CHECK_INT_EQUAL(sentry_value_get_length(spans), 2);

    sentry_value_decref(tx_other);

    sentry_close();
}

#undef IS_NULL
#undef CHECK_STRING_PROPERTY
