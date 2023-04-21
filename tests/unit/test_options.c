#include "sentry_options.h"
#include "sentry_string.h"
#include "sentry_testsupport.h"

#include "sentry_options.h"
#include "sentry_string.h"
#include "sentry_testsupport.h"

SENTRY_TEST(options_sdk_name_defaults)
{
    sentry_options_t *options = sentry_options_new();
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
    sentry_options_t *options = sentry_options_new();

    // when the sdk name is set to a custom string
    const char *sdk_name = "sentry.native.android.flutter";
    const int result = sentry_options_set_sdk_name(options, sdk_name);

    // both the sdk_name and user_agent should reflect this change
    TEST_CHECK_INT_EQUAL(result, 0);
    TEST_CHECK_STRING_EQUAL(sentry_options_get_sdk_name(options), sdk_name);

    sentry_stringbuilder_t sb;
    sentry__stringbuilder_init(&sb);
    sentry__stringbuilder_append(&sb, sdk_name);
    sentry__stringbuilder_append(&sb, "/");
    sentry__stringbuilder_append(&sb, SENTRY_SDK_VERSION);
    const char *expected_user_agent = sentry__stringbuilder_into_string(&sb);

    TEST_CHECK_STRING_EQUAL(
        sentry_options_get_user_agent(options), expected_user_agent);

    sentry_options_free(options);
}

SENTRY_TEST(options_sdk_name_invalid)
{
    sentry_options_t *options = sentry_options_new();

    // when the sdk name is set to an invalid value
    const char *sdk_name = NULL;
    const int result = sentry_options_set_sdk_name(options, sdk_name);

    // then the value should should be ignored
    TEST_CHECK_INT_EQUAL(result, 1);
    TEST_CHECK_STRING_EQUAL(
        sentry_options_get_sdk_name(options), SENTRY_SDK_NAME);
    TEST_CHECK_STRING_EQUAL(
        sentry_options_get_user_agent(options), SENTRY_SDK_USER_AGENT);

    sentry_options_free(options);
}
