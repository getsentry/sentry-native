#include "sentry_backend.h"
#include "sentry_core.h"
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

// Test helper to trigger a crash and verify before_crash_func is called
static void
trigger_crash_and_verify(const char *backend_name, void *user_data_marker)
{
    reset_before_crash_state();

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_before_crash(
        options, test_before_crash_func, user_data_marker);

    // Disable auto session tracking to avoid interference
    sentry_options_set_auto_session_tracking(options, false);

    // Initialize with the current backend
    TEST_CHECK_INT_EQUAL(sentry_init(options), 0);

    // Trigger a crash using sentry_capture_exception
    // This simulates a crash without actually crashing the test
    sentry_ucontext_t uctx = { 0 };

    SENTRY_WITH_OPTIONS (opts) {
        if (opts->backend && opts->backend->except_func) {
            opts->backend->except_func(opts->backend, &uctx);
        }
    }

    sentry_close();

    // Verify the before_crash_func was called
    TEST_CHECK_MSG(g_before_crash_called,
        "before_crash_func should be called with %s backend", backend_name);
    TEST_CHECK_INT_EQUAL_MSG(g_before_crash_call_count, 1,
        "before_crash_func should be called exactly once with %s backend",
        backend_name);
    TEST_CHECK_MSG(g_before_crash_user_data == user_data_marker,
        "before_crash_func should receive correct user_data with %s backend",
        backend_name);
}

SENTRY_TEST(before_crash_func_crashpad)
{
#ifdef SENTRY_BACKEND_CRASHPAD
    void *marker = (void *)0x12345678;
    trigger_crash_and_verify("crashpad", marker);
#else
    TEST_SKIP("Crashpad backend not available in this build");
#endif
}

SENTRY_TEST(before_crash_func_breakpad)
{
#ifdef SENTRY_BACKEND_BREAKPAD
    void *marker = (void *)0x87654321;
    trigger_crash_and_verify("breakpad", marker);
#else
    TEST_SKIP("Breakpad backend not available in this build");
#endif
}

SENTRY_TEST(before_crash_func_inproc)
{
#ifdef SENTRY_BACKEND_INPROC
    void *marker = (void *)0xABCDEF00;
    trigger_crash_and_verify("inproc", marker);
#else
    TEST_SKIP("In-process backend not available in this build");
#endif
}

// Test that before_crash_func is not called when not set
SENTRY_TEST(before_crash_func_not_set)
{
    reset_before_crash_state();

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    // Do not set before_crash_func
    sentry_options_set_auto_session_tracking(options, false);

    TEST_CHECK_INT_EQUAL(sentry_init(options), 0);

    // Trigger a crash
    sentry_ucontext_t uctx = { 0 };
    SENTRY_WITH_OPTIONS (opts) {
        if (opts->backend && opts->backend->except_func) {
            opts->backend->except_func(opts->backend, &uctx);
        }
    }

    sentry_close();

    // Verify the before_crash_func was NOT called
    TEST_CHECK_MSG(!g_before_crash_called,
        "before_crash_func should not be called when not set");
    TEST_CHECK_INT_EQUAL(g_before_crash_call_count, 0);
}

// Test that before_crash_func can be changed
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

    // Trigger a crash
    sentry_ucontext_t uctx = { 0 };
    SENTRY_WITH_OPTIONS (opts) {
        if (opts->backend && opts->backend->except_func) {
            opts->backend->except_func(opts->backend, &uctx);
        }
    }

    sentry_close();

    // Verify the updated user_data was used
    TEST_CHECK(g_before_crash_called);
    TEST_CHECK_MSG(g_before_crash_user_data == marker2,
        "before_crash_func should use the updated user_data");
}
