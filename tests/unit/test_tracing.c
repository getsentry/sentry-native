#include "sentry.h"
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

SENTRY_TEST(basic_transaction_context)
{
    sentry_value_t tx_cxt = sentry_value_new_transaction_context("");
    TEST_CHECK(!sentry_value_is_null(tx_cxt));
    const char *tx_name
        = sentry_value_as_string(sentry_value_get_by_key(tx_cxt, "name"));
    TEST_CHECK_STRING_EQUAL(tx_name, "<unlabeled transaction>");

    sentry_value_decref(tx_cxt);
    tx_cxt = sentry_value_new_transaction_context("honk.beep");
    tx_name = sentry_value_as_string(sentry_value_get_by_key(tx_cxt, "name"));
    TEST_CHECK_STRING_EQUAL(tx_name, "honk.beep");

    sentry_transaction_context_set_name(tx_cxt, "");
    tx_name = sentry_value_as_string(sentry_value_get_by_key(tx_cxt, "name"));
    TEST_CHECK_STRING_EQUAL(tx_name, "<unlabeled transaction>");

    sentry_transaction_context_set_operation(tx_cxt, "beepbeep");
    const char *tx_op
        = sentry_value_as_string(sentry_value_get_by_key(tx_cxt, "op"));
    TEST_CHECK_STRING_EQUAL(tx_op, "beepbeep");

    sentry_transaction_context_set_sampled(tx_cxt, 1);
    TEST_CHECK(
        sentry_value_is_true(sentry_value_get_by_key(tx_cxt, "sampled")) == 1);

    sentry_value_decref(tx_cxt);
}
