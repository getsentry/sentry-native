#include "sentry_alloc.h"
#include "sentry_backend.h"
#include "sentry_options.h"
#include "sentry_scope.h"
#include "sentry_testsupport.h"

#include <math.h>
#include <string.h>

static int
startup_with_initial_tags(
    sentry_backend_t *backend, const sentry_options_t *UNUSED(options))
{
    bool *found = backend->data;
    SENTRY_WITH_SCOPE (scope) {
        const char *value = sentry_value_as_string(
            sentry_value_get_by_key(scope->tags, "initial"));
        *found = value && strcmp(value, "value") == 0;
    }
    return 0;
}

static int
startup_failure_with_initial_tags(
    sentry_backend_t *backend, const sentry_options_t *options)
{
    startup_with_initial_tags(backend, options);
    return 1;
}

SENTRY_TEST(options_initial_tags_before_backend_startup)
{
    SENTRY_TEST_OPTIONS_NEW(options);

    bool found = false;
    sentry_backend_t *backend = SENTRY_MAKE(sentry_backend_t);
    TEST_ASSERT(!!backend);
    backend->data = &found;
    backend->startup_func = startup_with_initial_tags;
    sentry_options_set_backend(options, backend);

    sentry_value_t tags = sentry_value_new_object();
    sentry_value_set_by_key(tags, "initial", sentry_value_new_string("value"));
    sentry_value_set_by_key(tags, "invalid", sentry_value_new_int32(42));
    sentry_options_set_tags(options, tags);

    sentry_init(options);
    TEST_CHECK(found);

    SENTRY_WITH_SCOPE (scope) {
        TEST_CHECK_STRING_EQUAL(sentry_value_as_string(sentry_value_get_by_key(
                                    scope->tags, "initial")),
            "value");
        TEST_CHECK(sentry_value_is_null(
            sentry_value_get_by_key(scope->tags, "invalid")));
    }

    sentry_close();
}

SENTRY_TEST(options_initial_tags_rollback_after_startup_failure)
{
    SENTRY_TEST_OPTIONS_NEW(options);

    bool found = false;
    sentry_backend_t *backend = SENTRY_MAKE(sentry_backend_t);
    TEST_ASSERT(!!backend);
    backend->data = &found;
    backend->startup_func = startup_failure_with_initial_tags;
    sentry_options_set_backend(options, backend);

    sentry_value_t tags = sentry_value_new_object();
    sentry_value_set_by_key(tags, "initial", sentry_value_new_string("value"));
    sentry_options_set_tags(options, tags);

    TEST_CHECK(sentry_init(options) != 0);
    TEST_CHECK(found);

    SENTRY_WITH_SCOPE (scope) {
        TEST_CHECK(sentry_value_is_null(
            sentry_value_get_by_key(scope->tags, "initial")));
    }
}

SENTRY_TEST(options_initial_tags_replace)
{
    SENTRY_TEST_OPTIONS_NEW(options);

    sentry_value_t first = sentry_value_new_object();
    sentry_value_incref(first);
    sentry_options_set_tags(options, first);
    TEST_CHECK_INT_EQUAL(sentry_value_refcount(first), 2);

    sentry_value_t second = sentry_value_new_object();
    sentry_value_incref(second);
    sentry_options_set_tags(options, second);
    TEST_CHECK_INT_EQUAL(sentry_value_refcount(first), 1);
    TEST_CHECK_INT_EQUAL(sentry_value_refcount(second), 2);

    sentry_options_set_tags(options, sentry_value_new_list());
    TEST_CHECK(sentry_value_is_null(options->initial_scope_tags));
    TEST_CHECK_INT_EQUAL(sentry_value_refcount(second), 1);

    sentry_value_decref(first);
    sentry_value_decref(second);

    sentry_value_t final = sentry_value_new_object();
    sentry_value_incref(final);
    sentry_options_set_tags(options, final);
    sentry_options_free(options);
    TEST_CHECK_INT_EQUAL(sentry_value_refcount(final), 1);
    sentry_value_decref(final);
}

SENTRY_TEST(options_sdk_name_defaults)
{
    SENTRY_TEST_OPTIONS_NEW(options);
    // when nothing is set

    // then both sdk name and user agent should default to the build time
    // directives
    TEST_CHECK_STRING_EQUAL(
        sentry_options_get_sdk_name(options), SENTRY_SDK_NAME);
    TEST_CHECK_STRING_EQUAL(
        sentry_options_get_user_agent(options), SENTRY_SDK_USER_AGENT);

    sentry_options_free(options);
}

SENTRY_TEST(options_sdk_name_custom)
{
    SENTRY_TEST_OPTIONS_NEW(options);

    // when the sdk name is set to a custom string
    const int result
        = sentry_options_set_sdk_name(options, "sentry.native.android.flutter");

    // both the sdk_name and user_agent should reflect this change
    TEST_CHECK_INT_EQUAL(result, 0);
    TEST_CHECK_STRING_EQUAL(
        sentry_options_get_sdk_name(options), "sentry.native.android.flutter");

    TEST_CHECK_STRING_EQUAL(sentry_options_get_user_agent(options),
        "sentry.native.android.flutter/" SENTRY_SDK_VERSION);

    sentry_options_free(options);
}

SENTRY_TEST(options_sdk_name_invalid)
{
    SENTRY_TEST_OPTIONS_NEW(options);

    // when the sdk name is set to an invalid value
    const char *sdk_name = NULL;
    const int result = sentry_options_set_sdk_name(options, sdk_name);

    // then the value should be ignored
    TEST_CHECK_INT_EQUAL(result, 1);
    TEST_CHECK_STRING_EQUAL(
        sentry_options_get_sdk_name(options), SENTRY_SDK_NAME);
    TEST_CHECK_STRING_EQUAL(
        sentry_options_get_user_agent(options), SENTRY_SDK_USER_AGENT);

    sentry_options_free(options);
}

SENTRY_TEST(options_logger_enabled_when_crashed_default)
{
    SENTRY_TEST_OPTIONS_NEW(options);

    // Enabled by default
    TEST_CHECK_INT_EQUAL(options->enable_logging_when_crashed, 1);

    // Test setting to false
    sentry_options_set_logger_enabled_when_crashed(options, 0);
    TEST_CHECK_INT_EQUAL(options->enable_logging_when_crashed, 0);

    // Test setting to true
    sentry_options_set_logger_enabled_when_crashed(options, 1);
    TEST_CHECK_INT_EQUAL(options->enable_logging_when_crashed, 1);

    // Test setting with non-zero value (should be converted to 1)
    sentry_options_set_logger_enabled_when_crashed(options, 42);
    TEST_CHECK_INT_EQUAL(options->enable_logging_when_crashed, 1);

    sentry_options_free(options);
}

SENTRY_TEST(options_crash_reporting_mode_default)
{
    SENTRY_TEST_OPTIONS_NEW(options);

    // Default should be NATIVE_WITH_MINIDUMP (mode 3)
    TEST_CHECK_INT_EQUAL(sentry_options_get_crash_reporting_mode(options),
        SENTRY_CRASH_REPORTING_MODE_NATIVE_WITH_MINIDUMP);

    sentry_options_free(options);
}

SENTRY_TEST(options_crash_reporting_mode_set_get)
{
    SENTRY_TEST_OPTIONS_NEW(options);

    // Test setting to MINIDUMP mode
    sentry_options_set_crash_reporting_mode(
        options, SENTRY_CRASH_REPORTING_MODE_MINIDUMP);
    TEST_CHECK_INT_EQUAL(sentry_options_get_crash_reporting_mode(options),
        SENTRY_CRASH_REPORTING_MODE_MINIDUMP);

    // Test setting to NATIVE mode
    sentry_options_set_crash_reporting_mode(
        options, SENTRY_CRASH_REPORTING_MODE_NATIVE);
    TEST_CHECK_INT_EQUAL(sentry_options_get_crash_reporting_mode(options),
        SENTRY_CRASH_REPORTING_MODE_NATIVE);

    // Test setting to NATIVE_WITH_MINIDUMP mode
    sentry_options_set_crash_reporting_mode(
        options, SENTRY_CRASH_REPORTING_MODE_NATIVE_WITH_MINIDUMP);
    TEST_CHECK_INT_EQUAL(sentry_options_get_crash_reporting_mode(options),
        SENTRY_CRASH_REPORTING_MODE_NATIVE_WITH_MINIDUMP);

    sentry_options_free(options);
}

SENTRY_TEST(options_crash_reporting_mode_clamp)
{
    SENTRY_TEST_OPTIONS_NEW(options);

    // Test clamping invalid high values to NATIVE_WITH_MINIDUMP
    sentry_options_set_crash_reporting_mode(options, 99);
    TEST_CHECK_INT_EQUAL(sentry_options_get_crash_reporting_mode(options),
        SENTRY_CRASH_REPORTING_MODE_NATIVE_WITH_MINIDUMP);

    // Test clamping invalid low values to MINIDUMP
    sentry_options_set_crash_reporting_mode(options, -1);
    TEST_CHECK_INT_EQUAL(sentry_options_get_crash_reporting_mode(options),
        SENTRY_CRASH_REPORTING_MODE_MINIDUMP);

    sentry_options_free(options);
}

SENTRY_TEST(options_thread_stackwalk_mode_default)
{
    SENTRY_TEST_OPTIONS_NEW(options);

    TEST_CHECK_INT_EQUAL(sentry_options_get_thread_stackwalk_mode(options),
        SENTRY_THREAD_STACKWALK_MODE_ALL);

    sentry_options_free(options);
}

SENTRY_TEST(options_thread_stackwalk_mode_set_get)
{
    SENTRY_TEST_OPTIONS_NEW(options);

    sentry_options_set_thread_stackwalk_mode(
        options, SENTRY_THREAD_STACKWALK_MODE_CRASHED_ONLY);
    TEST_CHECK_INT_EQUAL(sentry_options_get_thread_stackwalk_mode(options),
        SENTRY_THREAD_STACKWALK_MODE_CRASHED_ONLY);

    sentry_options_set_thread_stackwalk_mode(
        options, SENTRY_THREAD_STACKWALK_MODE_ALL);
    TEST_CHECK_INT_EQUAL(sentry_options_get_thread_stackwalk_mode(options),
        SENTRY_THREAD_STACKWALK_MODE_ALL);

    sentry_options_free(options);
}

SENTRY_TEST(options_thread_stackwalk_mode_clamp)
{
    SENTRY_TEST_OPTIONS_NEW(options);

    // Test clamping invalid high values to ALL
    sentry_options_set_thread_stackwalk_mode(options, 99);
    TEST_CHECK_INT_EQUAL(sentry_options_get_thread_stackwalk_mode(options),
        SENTRY_THREAD_STACKWALK_MODE_ALL);

    // Test clamping invalid low values to CRASHED_ONLY
    sentry_options_set_thread_stackwalk_mode(options, -1);
    TEST_CHECK_INT_EQUAL(sentry_options_get_thread_stackwalk_mode(options),
        SENTRY_THREAD_STACKWALK_MODE_CRASHED_ONLY);

    // The reserved zero value clamps to CRASHED_ONLY as well
    sentry_options_set_thread_stackwalk_mode(options, 0);
    TEST_CHECK_INT_EQUAL(sentry_options_get_thread_stackwalk_mode(options),
        SENTRY_THREAD_STACKWALK_MODE_CRASHED_ONLY);

    sentry_options_free(options);
}

SENTRY_TEST(options_minidump_flags)
{
#ifdef SENTRY_PLATFORM_WINDOWS
    SENTRY_TEST_OPTIONS_NEW(options);

    TEST_CHECK_INT_EQUAL(options->minidump_flags, -1);
    TEST_CHECK_INT_EQUAL(options->minidump_mode, SENTRY_MINIDUMP_MODE_SMART);

    // Flags configured after a mode take precedence.
    sentry_options_set_minidump_mode(options, SENTRY_MINIDUMP_MODE_STACK_ONLY);
    sentry_options_set_minidump_flags(options, 0x00000002);
    TEST_CHECK_INT_EQUAL(
        options->minidump_mode, SENTRY_MINIDUMP_MODE_STACK_ONLY);
    TEST_CHECK_INT_EQUAL(options->minidump_flags, 0x00000002);

    // A later mode change does not clear custom flags.
    sentry_options_set_minidump_mode(options, SENTRY_MINIDUMP_MODE_FULL);
    TEST_CHECK_INT_EQUAL(options->minidump_mode, SENTRY_MINIDUMP_MODE_FULL);
    TEST_CHECK_INT_EQUAL(options->minidump_flags, 0x00000002);

    // Zero is a valid custom value corresponding to MiniDumpNormal.
    sentry_options_set_minidump_flags(options, 0);
    TEST_CHECK_INT_EQUAL(options->minidump_flags, 0);

    // The complete set of valid MINIDUMP_TYPE flags fits in the field.
    sentry_options_set_minidump_flags(options, 0x01ffffff);
    TEST_CHECK_INT_EQUAL(options->minidump_flags, 0x01ffffff);

    // Unsupported bits are discarded before storing the flags.
    sentry_options_set_minidump_flags(options, UINT32_MAX);
    TEST_CHECK_INT_EQUAL(options->minidump_flags, 0x01ffffff);

    sentry_options_free(options);
#else
    SKIP_TEST();
#endif
}

SENTRY_TEST(options_sample_rate)
{
    SENTRY_TEST_OPTIONS_NEW(options);

    sentry_options_set_sample_rate(options, 0.0);
    sentry_options_set_traces_sample_rate(options, 0.0);
    TEST_CHECK(sentry_options_get_sample_rate(options) == 0.0);
    TEST_CHECK(sentry_options_get_traces_sample_rate(options) == 0.0);

    sentry_options_set_sample_rate(options, 0.5);
    sentry_options_set_traces_sample_rate(options, 0.5);
    TEST_CHECK(sentry_options_get_sample_rate(options) == 0.5);
    TEST_CHECK(sentry_options_get_traces_sample_rate(options) == 0.5);

    sentry_options_set_sample_rate(options, 1.0);
    sentry_options_set_traces_sample_rate(options, 1.0);
    TEST_CHECK(sentry_options_get_sample_rate(options) == 1.0);
    TEST_CHECK(sentry_options_get_traces_sample_rate(options) == 1.0);

    // < 0.0
    sentry_options_set_sample_rate(options, -0.1);
    sentry_options_set_traces_sample_rate(options, -0.1);
    TEST_CHECK(sentry_options_get_sample_rate(options) == 0.0);
    TEST_CHECK(sentry_options_get_traces_sample_rate(options) == 0.0);

    // > 1.0
    sentry_options_set_sample_rate(options, 1.1);
    sentry_options_set_traces_sample_rate(options, 1.1);
    TEST_CHECK(sentry_options_get_sample_rate(options) == 1.0);
    TEST_CHECK(sentry_options_get_traces_sample_rate(options) == 1.0);

    // NaN -> default
    sentry_options_set_sample_rate(options, NAN);
    sentry_options_set_traces_sample_rate(options, NAN);
    TEST_CHECK(sentry_options_get_sample_rate(options) == 1.0);
    TEST_CHECK(sentry_options_get_traces_sample_rate(options) == 0.0);

    sentry_options_free(options);
}
