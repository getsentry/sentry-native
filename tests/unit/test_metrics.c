#include "sentry_metrics.h"
#include "sentry_testsupport.h"

#define TEST_CHECK_METRICS_SANITY(fn, input, expected)                         \
    {                                                                          \
        char *actual = fn(input);                                              \
        TEST_CHECK_STRING_EQUAL(actual, expected);                             \
        sentry_free(actual);                                                   \
    }

SENTRY_TEST(metrics_name_sanitize)
{
    TEST_CHECK_METRICS_SANITY(
        sentry__metrics_sanitize_name, "foo-bar", "foo-bar");
    TEST_CHECK_METRICS_SANITY(
        sentry__metrics_sanitize_name, "foo\\!,bar", "foo\\__bar");
    TEST_CHECK_METRICS_SANITY(
        sentry__metrics_sanitize_name, "fo%-bar", "fo_-bar");
    TEST_CHECK_METRICS_SANITY(
        sentry__metrics_sanitize_name, "fôo-bär", "f_o-b_r");
}

SENTRY_TEST(metrics_unit_sanitize)
{
    TEST_CHECK_METRICS_SANITY(sentry__metrics_sanitize_unit,
        "abcABC123_-./\\/%&abcABC12", "abcABC123_abcABC12");
}

SENTRY_TEST(metrics_tag_key_sanitize)
{
    TEST_CHECK_METRICS_SANITY(sentry__metrics_sanitize_tag_key,
        "a/weird/tag-key/:\\$", "a/weird/tag-key/\\");
}

SENTRY_TEST(metrics_tag_value_sanitize)
{
    TEST_CHECK_METRICS_SANITY(
        sentry__metrics_sanitize_tag_value, "plaintext", "plaintext");
    TEST_CHECK_METRICS_SANITY(
        sentry__metrics_sanitize_tag_value, "plain text", "plain text");
    TEST_CHECK_METRICS_SANITY(
        sentry__metrics_sanitize_tag_value, "plain%text", "plain%text");

    // Escape sequences
    TEST_CHECK_METRICS_SANITY(
        sentry__metrics_sanitize_tag_value, "plain \\ text", "plain \\\\ text");
    TEST_CHECK_METRICS_SANITY(
        sentry__metrics_sanitize_tag_value, "plain,text", "plain\\u{2c}text");
    TEST_CHECK_METRICS_SANITY(
        sentry__metrics_sanitize_tag_value, "plain|text", "plain\\u{7c}text");

    // Escapable control characters
    TEST_CHECK_METRICS_SANITY(
        sentry__metrics_sanitize_tag_value, "plain\ntext", "plain\\ntext");
    TEST_CHECK_METRICS_SANITY(
        sentry__metrics_sanitize_tag_value, "plain\rtext", "plain\\rtext");
    TEST_CHECK_METRICS_SANITY(
        sentry__metrics_sanitize_tag_value, "plain\ttext", "plain\\ttext");
}
