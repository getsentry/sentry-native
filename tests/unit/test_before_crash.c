#include "sentry_core.h"
#include "sentry_options.h"
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

SENTRY_TEST(before_crash_func_call)
{
    void *user_data = (void *)1;
    reset_before_crash_state();

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_before_crash(options, test_before_crash_func, user_data);
    sentry_options_set_auto_session_tracking(options, false);

    TEST_CHECK_INT_EQUAL(sentry_init(options), 0);

    // Verify before_crash_func is set correctly in options
    TEST_CHECK(options->before_crash_func == test_before_crash_func);
    TEST_CHECK(options->before_crash_data == user_data);

    // Call the before_crash_func directly to test it
    if (options->before_crash_func) {
        options->before_crash_func(options->before_crash_data);
    }

    sentry_close();

    // Verify the before_crash_func was called correctly
    TEST_CHECK(g_before_crash_called);
    TEST_CHECK_INT_EQUAL(g_before_crash_call_count, 1);
    TEST_CHECK(g_before_crash_user_data == user_data);
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
    TEST_CHECK(options->before_crash_func == NULL);

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
    void *user_data1 = (void *)2;
    sentry_options_set_before_crash(
        options, test_before_crash_func, user_data1);

    // Change to different user data
    void *user_data2 = (void *)3;
    sentry_options_set_before_crash(
        options, test_before_crash_func, user_data2);

    sentry_options_set_auto_session_tracking(options, false);
    TEST_CHECK_INT_EQUAL(sentry_init(options), 0);

    // Verify the updated user_data is set
    TEST_CHECK(options->before_crash_func == test_before_crash_func);
    TEST_CHECK(options->before_crash_data == user_data2);

    // Call the before_crash_func to test it
    if (options->before_crash_func) {
        options->before_crash_func(options->before_crash_data);
    }

    sentry_close();

    // Verify the updated user_data was used
    TEST_CHECK(g_before_crash_called);
    TEST_CHECK(g_before_crash_user_data == user_data2);
}
