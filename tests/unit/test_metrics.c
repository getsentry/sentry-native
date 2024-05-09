#include "sentry_testsupport.h"
#include "sentry_metrics.h"

SENTRY_TEST(metrics_tag_value_sanitize)
{
    TEST_CHECK_STRING_EQUAL(
        sentry__metrics_sanitize_tag_value("plain"), "plain");
    TEST_CHECK_STRING_EQUAL(
        sentry__metrics_sanitize_tag_value("plain text"), "plain text");
    TEST_CHECK_STRING_EQUAL(
        sentry__metrics_sanitize_tag_value("plain%text"), "plain%text");

    // Escape sequences
    TEST_CHECK_STRING_EQUAL(
        sentry__metrics_sanitize_tag_value("plain \\ text"), "plain \\\\ text");
    TEST_CHECK_STRING_EQUAL(
        sentry__metrics_sanitize_tag_value("plain,text"), "plain\\u{2c}text");
    TEST_CHECK_STRING_EQUAL(
        sentry__metrics_sanitize_tag_value("plain|text"), "plain\\u{7c}text");

    // Escapable control characters
    TEST_CHECK_STRING_EQUAL(
        sentry__metrics_sanitize_tag_value("plain\ntext"), "plain\\ntext");
    TEST_CHECK_STRING_EQUAL(
        sentry__metrics_sanitize_tag_value("plain\rtext"), "plain\\rtext");
    TEST_CHECK_STRING_EQUAL(
        sentry__metrics_sanitize_tag_value("plain\ttext"), "plain\\ttext");
}

