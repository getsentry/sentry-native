#include "sentry_testsupport.h"
#include <stdbool.h>

static volatile bool g_before_crash_called = false;
static volatile int g_before_crash_call_count = 0;
static volatile void *g_before_crash_user_data = NULL;

static void
test_before_crash_func(void *user_data)
{
    g_before_crash_called = true;
    g_before_crash_call_count++;
    g_before_crash_user_data = user_data;
}

static void
reset_before_crash_state(void)
{
    g_before_crash_called = false;
    g_before_crash_call_count = 0;
    g_before_crash_user_data = NULL;
}

SENTRY_TEST(before_crash_func_crashpad)
{
#ifdef SENTRY_BACKEND_CRASHPAD
    reset_before_crash_state();

    void *marker = (void *)0x12345678;
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_before_crash(options, test_before_crash_func, marker);
    sentry_options_set_auto_session_tracking(options, false);

    TEST_CHECK_INT_EQUAL(sentry_init(options), 0);

    // Verify before_crash_func is set correctly in options
    SENTRY_WITH_OPTIONS (opts) {
        TEST_CHECK(opts->before_crash_func == test_before_crash_func);
        TEST_CHECK(opts->before_crash_data == marker);

        // Call the before_crash_func directly to test it
        if (opts->before_crash_func) {
            opts->before_crash_func(opts->before_crash_data);
        }
    }

    sentry_close();

    // Verify the before_crash_func was called correctly
    TEST_CHECK(g_before_crash_called);
    TEST_CHECK_INT_EQUAL(g_before_crash_call_count, 1);
    TEST_CHECK(g_before_crash_user_data == marker);
#else
    SKIP_TEST();
#endif
}

SENTRY_TEST(before_crash_func_breakpad)
{
#ifdef SENTRY_BACKEND_BREAKPAD
    reset_before_crash_state();

    void *marker = (void *)0x87654321;
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_before_crash(options, test_before_crash_func, marker);
    sentry_options_set_auto_session_tracking(options, false);

    TEST_CHECK_INT_EQUAL(sentry_init(options), 0);

    // Verify before_crash_func is set correctly in options
    SENTRY_WITH_OPTIONS (opts) {
        TEST_CHECK(opts->before_crash_func == test_before_crash_func);
        TEST_CHECK(opts->before_crash_data == marker);

        // Call the before_crash_func directly to test it
        if (opts->before_crash_func) {
            opts->before_crash_func(opts->before_crash_data);
        }
    }

    sentry_close();

    // Verify the before_crash_func was called correctly
    TEST_CHECK(g_before_crash_called);
    TEST_CHECK_INT_EQUAL(g_before_crash_call_count, 1);
    TEST_CHECK(g_before_crash_user_data == marker);
#else
    SKIP_TEST();
#endif
}

SENTRY_TEST(before_crash_func_inproc)
{
#ifdef SENTRY_BACKEND_INPROC
    reset_before_crash_state();

    void *marker = (void *)0xABCDEF00;
    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_before_crash(options, test_before_crash_func, marker);
    sentry_options_set_auto_session_tracking(options, false);

    TEST_CHECK_INT_EQUAL(sentry_init(options), 0);

    // Verify before_crash_func is set correctly in options
    SENTRY_WITH_OPTIONS (opts) {
        TEST_CHECK(opts->before_crash_func == test_before_crash_func);
        TEST_CHECK(opts->before_crash_data == marker);

        // Call the before_crash_func directly to test it
        if (opts->before_crash_func) {
            opts->before_crash_func(opts->before_crash_data);
        }
    }

    sentry_close();

    // Verify the before_crash_func was called correctly
    TEST_CHECK(g_before_crash_called);
    TEST_CHECK_INT_EQUAL(g_before_crash_call_count, 1);
    TEST_CHECK(g_before_crash_user_data == marker);
#else
    SKIP_TEST();
#endif
}

SENTRY_TEST(before_crash_func_not_set)
{
    reset_before_crash_state();

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    // Do not set before_crash_func
    sentry_options_set_auto_session_tracking(options, false);

    TEST_CHECK_INT_EQUAL(sentry_init(options), 0);

    // Verify before_crash_func is not set
    SENTRY_WITH_OPTIONS (opts) {
        TEST_CHECK(opts->before_crash_func == NULL);
    }

    sentry_close();

    // Verify the before_crash_func was not called
    TEST_CHECK(!g_before_crash_called);
    TEST_CHECK_INT_EQUAL(g_before_crash_call_count, 0);
}

SENTRY_TEST(before_crash_func_change)
{
    reset_before_crash_state();

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");

    // Set initial before_crash_func
    void *marker1 = (void *)0x11111111;
    sentry_options_set_before_crash(options, test_before_crash_func, marker1);

    // Change to different user data
    void *marker2 = (void *)0x22222222;
    sentry_options_set_before_crash(options, test_before_crash_func, marker2);

    sentry_options_set_auto_session_tracking(options, false);
    TEST_CHECK_INT_EQUAL(sentry_init(options), 0);

    // Verify the updated user_data is set
    SENTRY_WITH_OPTIONS (opts) {
        TEST_CHECK(opts->before_crash_func == test_before_crash_func);
        TEST_CHECK(opts->before_crash_data == marker2);

        // Call the before_crash_func to test it
        if (opts->before_crash_func) {
            opts->before_crash_func(opts->before_crash_data);
        }
    }

    sentry_close();

    // Verify the updated user_data was used
    TEST_CHECK(g_before_crash_called);
    TEST_CHECK(g_before_crash_user_data == marker2);
}
