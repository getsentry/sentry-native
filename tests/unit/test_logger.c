#include "sentry_core.h"
#include "sentry_logger.h"
#include "sentry_sync.h"
#include "sentry_testsupport.h"

typedef struct {
    uint64_t called;
    bool assert_now;
} logger_test_t;

static void
test_logger(
    sentry_level_t level, const char *message, va_list args, void *_data)
{
    logger_test_t *data = _data;
    if (data->assert_now) {
        data->called += 1;

        TEST_CHECK(level == SENTRY_LEVEL_WARNING);

        char formatted[128];
        vsnprintf(formatted, sizeof(formatted), message, args);

        TEST_CHECK_STRING_EQUAL(formatted, "Oh this is bad");
    }
}

SENTRY_TEST(custom_logger)
{
    logger_test_t data = { 0, false };

    {
        SENTRY_TEST_OPTIONS_NEW(options);
        sentry_options_set_debug(options, true);
        sentry_options_set_logger(options, test_logger, &data);

        sentry_init(options);

        data.assert_now = true;
        SENTRY_WARNF("Oh this is %s", "bad");
        data.assert_now = false;

        sentry_close();
    }

    TEST_CHECK_INT_EQUAL(data.called, 1);

    {
        // *really* clear the logger instance
        SENTRY_TEST_OPTIONS_NEW(options);
        sentry_init(options);
        sentry_close();
    }
}

SENTRY_TEST(logger_enable_disable_functionality)
{
    logger_test_t data = { 0, false };

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_debug(options, true);
    sentry_options_set_logger(options, test_logger, &data);

    sentry_init(options);

    // Test logging is enabled by default
    data.called = 0;
    data.assert_now = true;
    SENTRY_WARNF("Oh this is %s", "bad");
    TEST_CHECK_INT_EQUAL(data.called, 1);

    // Test disabling logging
    sentry__logger_disable();
    data.called = 0;
    data.assert_now = false;
    SENTRY_WARNF("Don't log %s", "this");
    TEST_CHECK_INT_EQUAL(data.called, 0);

    // Test re-enabling logging
    sentry__logger_enable();
    data.called = 0;
    data.assert_now = true;
    SENTRY_WARNF("Oh this is %s", "bad");
    TEST_CHECK_INT_EQUAL(data.called, 1);
    data.assert_now = false;

    // Clear the logger instance
    SENTRY_TEST_OPTIONS_NEW(clean_options);
    sentry_init(clean_options);
    sentry_close();
}
