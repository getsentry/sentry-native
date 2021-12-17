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

    sentry_value_t transaction = sentry_envelope_get_transaction(envelope);
    TEST_CHECK(!sentry_value_is_null(transaction));
    CHECK_STRING_PROPERTY(
        transaction, "transaction", "<unlabeled transaction>");

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

    sentry_value_t transaction
        = sentry_value_new_transaction_context(NULL, NULL);
    sentry_transaction_start(transaction);
    sentry_uuid_t event_id = sentry_transaction_finish();
    TEST_CHECK(!sentry_uuid_is_nil(&event_id));

    transaction = sentry_value_new_transaction_context("", "");
    sentry_transaction_start(transaction);
    event_id = sentry_transaction_finish();
    TEST_CHECK(!sentry_uuid_is_nil(&event_id));

    sentry_close();
    TEST_CHECK_INT_EQUAL(called, 2);
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

    sentry_value_t transaction = sentry_value_new_transaction_context(
        "How could you", "Don't capture this.");
    sentry_transaction_start(transaction);
    sentry_uuid_t event_id = sentry_transaction_finish();
    // TODO: `sentry_capture_event` acts as if the event was sent if user
    // consent was not given
    TEST_CHECK(!sentry_uuid_is_nil(&event_id));
    sentry_user_consent_give();

    transaction = sentry_value_new_transaction_context("honk", "beep");
    sentry_transaction_start(transaction);
    event_id = sentry_transaction_finish();
    TEST_CHECK(!sentry_uuid_is_nil(&event_id));

    sentry_user_consent_revoke();
    transaction = sentry_value_new_transaction_context(
        "How could you again", "Don't capture this either.");
    sentry_transaction_start(transaction);
    event_id = sentry_transaction_finish();
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
        sentry_value_t transaction
            = sentry_value_new_transaction_context("honk", "beep");
        sentry_transaction_start(transaction);
        sentry_uuid_t event_id = sentry_transaction_finish();
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

    sentry_value_t transaction
        = sentry_value_new_transaction_context("honk", "beep");
    sentry_transaction_start(transaction);
    sentry_uuid_t event_id = sentry_transaction_finish();
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
    sentry_transaction_start(tx_cxt);

    sentry_value_t scope_tx = sentry__scope_get_span();
    CHECK_STRING_PROPERTY(scope_tx, "transaction", "wow!");

    sentry_uuid_t event_id = sentry_transaction_finish();
    scope_tx = sentry__scope_get_span();
    TEST_CHECK(sentry_value_is_null(scope_tx));
    TEST_CHECK(!sentry_uuid_is_nil(&event_id));

    tx_cxt = sentry_value_new_transaction_context("whoa!", NULL);
    sentry_transaction_start(tx_cxt);
    tx_cxt = sentry_value_new_transaction_context("wowee!", NULL);
    sentry_transaction_start(tx_cxt);
    scope_tx = sentry__scope_get_span();
    CHECK_STRING_PROPERTY(scope_tx, "transaction", "wowee!");
    event_id = sentry_transaction_finish();
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

    // Starting a child with no active transaction should fail
    sentry_value_t parentless_child
        = sentry_span_start_child(sentry_value_new_null(), NULL, NULL);
    TEST_CHECK(sentry_value_is_null(parentless_child));

    sentry_value_t tx_cxt = sentry_value_new_transaction_context("wow!", NULL);
    sentry_transaction_start(tx_cxt);

    sentry_value_t child
        = sentry_span_start_child(sentry_value_new_null(), "honk", "goose");
    TEST_CHECK(!sentry_value_is_null(child));

    // Peek into the transaction's span list and make sure everything is
    // good
    sentry_value_t scope_tx = sentry__scope_get_span();
    const char *trace_id
        = sentry_value_as_string(sentry_value_get_by_key(scope_tx, "trace_id"));
    const char *parent_span_id
        = sentry_value_as_string(sentry_value_get_by_key(scope_tx, "span_id"));
    TEST_CHECK(!IS_NULL(scope_tx, "spans"));
    TEST_CHECK_INT_EQUAL(
        sentry_value_get_length(sentry_value_get_by_key(scope_tx, "spans")), 1);

    // Make sure the span inherited everything correctly
    sentry_value_t stored_child = sentry_value_get_by_index(
        sentry_value_get_by_key(scope_tx, "spans"), 0);
    CHECK_STRING_PROPERTY(stored_child, "trace_id", trace_id);
    CHECK_STRING_PROPERTY(stored_child, "parent_span_id", parent_span_id);
    CHECK_STRING_PROPERTY(stored_child, "op", "honk");
    CHECK_STRING_PROPERTY(stored_child, "description", "goose");
    // Not finished yet
    TEST_CHECK(IS_NULL(stored_child, "timestamp"));
    // Span contexts carry indices in this SDK to make it easier to find and
    // update them, make sure they don't leak into the transaction
    TEST_CHECK(IS_NULL(stored_child, "index"));

    sentry_span_finish(child);
    stored_child = sentry_value_get_by_index(
        sentry_value_get_by_key(scope_tx, "spans"), 0);
    // Should be finished
    TEST_CHECK(!IS_NULL(stored_child, "timestamp"));

    sentry_close();
}

SENTRY_TEST(child_spans)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_traces_sample_rate(options, 1.0);
    sentry_options_set_max_spans(options, 3);
    sentry_init(options);

    // Starting a child with no active transaction should fail
    sentry_value_t parentless_child
        = sentry_span_start_child(sentry_value_new_null(), NULL, NULL);
    TEST_CHECK(sentry_value_is_null(parentless_child));

    // Finishing a nonexistent span doesn't explode anything
    sentry_value_t fake_span
        = sentry__value_new_span(sentry_value_new_null(), NULL);
    sentry_span_finish(fake_span);

    sentry_value_t tx_cxt = sentry_value_new_transaction_context("wow!", NULL);
    sentry_transaction_start(tx_cxt);

    sentry_value_t child
        = sentry_span_start_child(sentry_value_new_null(), "honk", "goose");
    TEST_CHECK(!sentry_value_is_null(child));

    // Peek into the transaction's span list and make sure everything is
    // good
    sentry_value_t scope_tx = sentry__scope_get_span();
    const char *trace_id
        = sentry_value_as_string(sentry_value_get_by_key(scope_tx, "trace_id"));
    TEST_CHECK_INT_EQUAL(
        sentry_value_get_length(sentry_value_get_by_key(scope_tx, "spans")), 1);

    const char *parent_span_id
        = sentry_value_as_string(sentry_value_get_by_key(child, "span_id"));

    sentry_value_t grandchild = sentry_span_start_child(child, "beep", "car");
    sentry_span_finish(grandchild);

    // Make sure everything on the transaction looks good, check grandchild
    TEST_CHECK_INT_EQUAL(
        sentry_value_get_length(sentry_value_get_by_key(scope_tx, "spans")), 2);

    sentry_value_t stored_grandchild = sentry_value_get_by_index(
        sentry_value_get_by_key(scope_tx, "spans"), 1);
    CHECK_STRING_PROPERTY(stored_grandchild, "trace_id", trace_id);
    CHECK_STRING_PROPERTY(stored_grandchild, "parent_span_id", parent_span_id);
    CHECK_STRING_PROPERTY(stored_grandchild, "op", "beep");
    CHECK_STRING_PROPERTY(stored_grandchild, "description", "car");
    // No span context-exclusive values leaking into transaction's spans
    TEST_CHECK(IS_NULL(stored_grandchild, "index"));
    // Should be finished
    TEST_CHECK(!IS_NULL(stored_grandchild, "timestamp"));

    sentry_span_finish(child);

    sentry_close();
}

SENTRY_TEST(overflow_spans)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_traces_sample_rate(options, 1.0);
    sentry_options_set_max_spans(options, 1);
    sentry_init(options);

    sentry_value_t tx_cxt = sentry_value_new_transaction_context("wow!", NULL);
    sentry_transaction_start(tx_cxt);

    sentry_value_t child
        = sentry_span_start_child(sentry_value_new_null(), "honk", "goose");
    const char *child_span_id
        = sentry_value_as_string(sentry_value_get_by_key(child, "span_id"));

    sentry_value_t scope_tx = sentry__scope_get_span();
    TEST_CHECK_INT_EQUAL(
        sentry_value_get_length(sentry_value_get_by_key(scope_tx, "spans")), 1);

    sentry_value_t overflow_child
        = sentry_span_start_child(child, "beep", "car");
    TEST_CHECK(sentry_value_is_null(overflow_child));

    TEST_CHECK_INT_EQUAL(
        sentry_value_get_length(sentry_value_get_by_key(scope_tx, "spans")), 1);

    sentry_value_t stored_child = sentry_value_get_by_index(
        sentry_value_get_by_key(scope_tx, "spans"), 0);
    CHECK_STRING_PROPERTY(stored_child, "span_id", child_span_id);

    sentry_value_decref(child);
    sentry_value_decref(overflow_child);

    sentry_close();
}

static void
check_spans(sentry_envelope_t *envelope, void *data)
{
    uint64_t *called = data;
    *called += 1;

    sentry_value_t transaction = sentry_envelope_get_transaction(envelope);
    TEST_CHECK(!sentry_value_is_null(transaction));

    size_t span_count = sentry_value_get_length(
        sentry_value_get_by_key(transaction, "spans"));
    TEST_CHECK_INT_EQUAL(span_count, 1);

    sentry_envelope_free(envelope);
}

SENTRY_TEST(drop_unfinished_spans)
{
    uint64_t called_transport = 0;

    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");

    sentry_transport_t *transport = sentry_transport_new(check_spans);
    sentry_transport_set_state(transport, &called_transport);
    sentry_options_set_transport(options, transport);

    sentry_options_set_traces_sample_rate(options, 1.0);
    sentry_options_set_max_spans(options, 2);
    sentry_init(options);

    sentry_value_t tx_cxt = sentry_value_new_transaction_context("wow!", NULL);
    sentry_transaction_start(tx_cxt);

    sentry_value_t child
        = sentry_span_start_child(sentry_value_new_null(), "honk", "goose");
    TEST_CHECK(!sentry_value_is_null(child));

    sentry_value_t grandchild = sentry_span_start_child(child, "beep", "car");
    TEST_CHECK(!sentry_value_is_null(grandchild));
    sentry_span_finish(grandchild);

    sentry_value_t scope_tx = sentry__scope_get_span();
    TEST_CHECK_INT_EQUAL(
        sentry_value_get_length(sentry_value_get_by_key(scope_tx, "spans")), 2);

    sentry_uuid_t event_id = sentry_transaction_finish();
    TEST_CHECK(!sentry_uuid_is_nil(&event_id));

    sentry_value_decref(child);

    sentry_close();

    TEST_CHECK_INT_EQUAL(called_transport, 1);
}

#undef IS_NULL
#undef CHECK_STRING_PROPERTY
