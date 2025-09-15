#include "sentry_logs.h"
#include "sentry_testsupport.h"

#include "sentry_envelope.h"

#ifdef SENTRY_PLATFORM_WINDOWS
#    include <windows.h>
#    define sleep_ms(MILLISECONDS) Sleep(MILLISECONDS)
#else
#    include <unistd.h>
#    define sleep_ms(MILLISECONDS) usleep(MILLISECONDS * 1000)
#endif

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

SENTRY_TEST(basic_logging_functionality)
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
    // TODO if we don't sleep, log timer_task might not start in time to flush
    sleep_ms(20);

    // These should not crash and should respect the enable_logs option
    sentry_log_trace("Trace message");
    sentry_log_debug("Debug message");
    sentry_log_info("Info message");
    sentry_log_warn("Warning message");
    sentry_log_error("Error message");
    // sleep to finish flush of the first 5, otherwise failed enqueue
    sleep_ms(20);
    sentry_log_fatal("Fatal message");
    sentry_close();

    // TODO for now we set unit test buffer size to 5; does this make sense?
    //  Or should we just pump out 100+ logs to fill a batch in a for-loop?
    TEST_CHECK_INT_EQUAL(called_transport, 2);
}

SENTRY_TEST(logs_disabled_by_default)
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

    sentry_log_info("This should not be sent");

    sentry_close();

    // Transport should not be called since logs are disabled
    TEST_CHECK_INT_EQUAL(called_transport, 0);
}

SENTRY_TEST(formatted_log_messages)
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

    // Test format specifiers
    sentry_log_info("String: %s, Integer: %d, Float: %.2f", "test", 42, 3.14);
    sentry_log_warn("Character: %c, Hex: 0x%x", 'A', 255);
    sentry_log_error("Pointer: %p", (void *)0x1234);
    sentry_log_error("Big number: %zu", UINT64_MAX);
    sentry_log_error("Small number: %d", INT64_MIN);

    sentry_close();

    // Transport should be called once
    TEST_CHECK_INT_EQUAL(called_transport, 1);
}

static void
test_param_conversion_helper(const char *format, ...)
{
    sentry_value_t attributes = sentry_value_new_object();
    va_list args;
    va_start(args, format);
    int param_count = populate_message_parameters(attributes, format, args);
    va_end(args);

    // Verify we got the expected number of parameters
    TEST_CHECK_INT_EQUAL(param_count, 3);

    // Verify the parameters were extracted correctly
    sentry_value_t param0
        = sentry_value_get_by_key(attributes, "sentry.message.parameter.0");
    sentry_value_t param1
        = sentry_value_get_by_key(attributes, "sentry.message.parameter.1");
    sentry_value_t param2
        = sentry_value_get_by_key(attributes, "sentry.message.parameter.2");

    TEST_CHECK(!sentry_value_is_null(param0));
    TEST_CHECK(!sentry_value_is_null(param1));
    TEST_CHECK(!sentry_value_is_null(param2));

    // Check the values
    sentry_value_t value0 = sentry_value_get_by_key(param0, "value");
    sentry_value_t value1 = sentry_value_get_by_key(param1, "value");
    sentry_value_t value2 = sentry_value_get_by_key(param2, "value");

    TEST_CHECK_INT_EQUAL(sentry_value_as_int64(value0), 1);
    TEST_CHECK_INT_EQUAL(sentry_value_as_int64(value1), 2);
    TEST_CHECK_INT_EQUAL(sentry_value_as_int64(value2), 3);

    sentry_value_decref(attributes);
}

SENTRY_TEST(logs_param_conversion)
{
    // TODO this test shows the current limitation for parsing integers on
    //  32-bit systems
    int a = 1, b = 2, c = 3;
#if defined(__i386__) || defined(_M_IX86) || defined(__arm__)
    // Currently, on 32-bit platforms, we need to cast to long long since the
    // parameter conversion expects long long for %d/%i format specifiers
    test_param_conversion_helper(
        "%" PRId64 " %" PRId64 " %" PRId64, (int64_t)a, (int64_t)b, (int64_t)c);
#else
    test_param_conversion_helper("%d %d %d", a, b, c);
#endif
}
