#include "sentry_logs.h"
#include "sentry_testsupport.h"

#include "sentry_envelope.h"

// TODO these tests will need updating after batching is implemented
// TODO reduce to bare minimum test setup
static void
validate_logs_envelope(sentry_envelope_t *envelope, void *data)
{
    uint64_t *called = data;
    *called += 1;

    // Verify we have at least one envelope item
    TEST_CHECK(sentry__envelope_get_item_count(envelope) > 0);

    // Get the first item and check it's a logs item
    const sentry_envelope_item_t *item = sentry__envelope_get_item(envelope, 0);
    sentry_value_t type_header = sentry__envelope_item_get_header(item, "type");
    TEST_CHECK_STRING_EQUAL(sentry_value_as_string(type_header), "log");

    sentry_envelope_free(envelope);
}

// Helper function to create a parameter object
static sentry_value_t
create_parameter(const char *name, sentry_value_t value)
{
    sentry_value_t param = sentry_value_new_object();
    sentry_value_set_by_key(param, "name", sentry_value_new_string(name));
    sentry_value_set_by_key(param, "value", value);
    return param;
}

SENTRY_TEST(basic_logging_functionality_value_t)
{
    uint64_t called_transport = 0;

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_enable_logs(options, true);

    sentry_transport_t *transport
        = sentry_transport_new(validate_logs_envelope);
    sentry_transport_set_state(transport, &called_transport);
    sentry_options_set_transport(options, transport);

    sentry_init(options);

    // Create empty parameter list for basic tests
    sentry_value_t empty_params = sentry_value_new_list();

    // These should not crash and should respect the enable_logs option
    sentry_log_trace_value("Trace message", empty_params);
    sentry_log_debug_value("Debug message", empty_params);
    sentry_log_info_value("Info message", empty_params);
    sentry_log_warn_value("Warning message", empty_params);
    sentry_log_error_value("Error message", empty_params);
    sentry_log_fatal_value("Fatal message", empty_params);

    sentry_value_decref(empty_params);
    sentry_close();

    TEST_CHECK_INT_EQUAL(called_transport, 6);
}

SENTRY_TEST(logs_disabled_by_default_value_t)
{
    uint64_t called_transport = 0;

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");

    sentry_transport_t *transport
        = sentry_transport_new(validate_logs_envelope);
    sentry_transport_set_state(transport, &called_transport);
    sentry_options_set_transport(options, transport);

    // Don't explicitly enable logs - they should be disabled by default
    sentry_init(options);

    sentry_value_t empty_params = sentry_value_new_list();
    sentry_log_info_value("This should not be sent", empty_params);
    sentry_value_decref(empty_params);

    sentry_close();

    // Transport should not be called since logs are disabled
    TEST_CHECK_INT_EQUAL(called_transport, 0);
}

SENTRY_TEST(formatted_log_messages_value_t)
{
    uint64_t called_transport = 0;

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_enable_logs(options, true);

    sentry_transport_t *transport
        = sentry_transport_new(validate_logs_envelope);
    sentry_transport_set_state(transport, &called_transport);
    sentry_options_set_transport(options, transport);

    sentry_init(options);

    // Test with string, integer, and float parameters
    sentry_value_t params1 = sentry_value_new_list();
    sentry_value_append(params1,
        create_parameter("str_param", sentry_value_new_string("test")));
    sentry_value_append(
        params1, create_parameter("int_param", sentry_value_new_int32(42)));
    sentry_value_append(params1,
        create_parameter("float_param", sentry_value_new_double(3.14)));

    sentry_log_info_value("String: %s, Integer: %d, Float: %.2f", params1);

    sentry_value_decref(params1);

    sentry_close();

    // Transport should be called three times
    TEST_CHECK_INT_EQUAL(called_transport, 1);
}

SENTRY_TEST(parameter_mismatch_value_t)
{
    uint64_t called_transport = 0;

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_enable_logs(options, true);

    sentry_transport_t *transport
        = sentry_transport_new(validate_logs_envelope);
    sentry_transport_set_state(transport, &called_transport);
    sentry_options_set_transport(options, transport);

    sentry_init(options);

    // Test with more format specifiers than parameters
    sentry_value_t params1 = sentry_value_new_list();
    sentry_value_append(
        params1, create_parameter("param1", sentry_value_new_int32(42)));
    sentry_log_info_value("I have one parameter %d but expect two %s", params1);

    // Test with more parameters than format specifiers
    sentry_value_t params2 = sentry_value_new_list();
    sentry_value_append(
        params2, create_parameter("param1", sentry_value_new_int32(1)));
    sentry_value_append(
        params2, create_parameter("param2", sentry_value_new_int32(2)));
    sentry_value_append(
        params2, create_parameter("param3", sentry_value_new_int32(3)));
    sentry_log_info_value(
        "I have three parameters but only use one %d", params2);

    // Test with no parameters but format specifiers
    sentry_value_t empty_params = sentry_value_new_list();
    sentry_log_info_value(
        "I expect a parameter %d but have none", empty_params);

    sentry_value_decref(params1);
    sentry_value_decref(params2);
    sentry_value_decref(empty_params);

    sentry_close();

    // Transport should be called three times
    TEST_CHECK_INT_EQUAL(called_transport, 3);
}
