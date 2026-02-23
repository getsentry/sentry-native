#include "sentry_options.h"
#include "sentry_string.h"
#include "sentry_testsupport.h"
#include <stdlib.h>

static char *
pushenv(const char *key, const char *value)
{
    char *old = sentry__string_clone(getenv(key));
#ifdef SENTRY_PLATFORM_WINDOWS
    _putenv_s(key, value);
#else
    setenv(key, value, 1);
#endif
    return old;
}

static void
popenv(const char *key, char *old)
{
    if (old) {
#ifdef SENTRY_PLATFORM_WINDOWS
        _putenv_s(key, old);
#else
        setenv(key, old, 1);
#endif
        sentry_free(old);
    } else {
#ifdef SENTRY_PLATFORM_WINDOWS
        _putenv_s(key, "");
#else
        unsetenv(key);
#endif
    }
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

SENTRY_TEST(options_sample_rate_env)
{
    char *old_sr = pushenv("SENTRY_SAMPLE_RATE", " 0.5 ");
    char *old_tsr = pushenv("SENTRY_TRACES_SAMPLE_RATE", "0.25\t");

    sentry_options_t *options = sentry_options_new();
    TEST_ASSERT(!!options);
    TEST_CHECK(sentry_options_get_sample_rate(options) == 0.5);
    TEST_CHECK(sentry_options_get_traces_sample_rate(options) == 0.25);
    sentry_options_free(options);

    popenv("SENTRY_SAMPLE_RATE", old_sr);
    popenv("SENTRY_TRACES_SAMPLE_RATE", old_tsr);
}

SENTRY_TEST(options_sample_rate_env_invalid)
{
    char *old_sr = pushenv("SENTRY_SAMPLE_RATE", "not_a_number");
    char *old_tsr = pushenv("SENTRY_TRACES_SAMPLE_RATE", "");

    sentry_options_t *options = sentry_options_new();
    TEST_ASSERT(!!options);
    TEST_CHECK(sentry_options_get_sample_rate(options) == 1.0);
    TEST_CHECK(sentry_options_get_traces_sample_rate(options) == 0.0);
    sentry_options_free(options);

    popenv("SENTRY_SAMPLE_RATE", old_sr);
    popenv("SENTRY_TRACES_SAMPLE_RATE", old_tsr);
}

SENTRY_TEST(options_sample_rate_nan)
{
    char *old_sr = pushenv("SENTRY_SAMPLE_RATE", "NaN");
    char *old_tsr = pushenv("SENTRY_TRACES_SAMPLE_RATE", "NaN");

    sentry_options_t *options = sentry_options_new();
    TEST_ASSERT(!!options);
    TEST_CHECK(sentry_options_get_sample_rate(options) == 1.0);
    TEST_CHECK(sentry_options_get_traces_sample_rate(options) == 0.0);
    sentry_options_free(options);

    popenv("SENTRY_SAMPLE_RATE", old_sr);
    popenv("SENTRY_TRACES_SAMPLE_RATE", old_tsr);
}
