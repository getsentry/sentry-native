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

    char *span_op
        = sentry_value_as_string(sentry_value_get_by_key(trace_context, "op"));
    TEST_CHECK_STRING_EQUAL(span_op, "honk.beep");
}
