#include "sentry_metrics.h"
#include "sentry_testsupport.h"

SENTRY_TEST(metrics_tag_value_sanitize)
{
    char *s1 = sentry__metrics_sanitize_tag_value("plain");
    char *s2 = sentry__metrics_sanitize_tag_value("plain text");
    char *s3 = sentry__metrics_sanitize_tag_value("plain%text");

    TEST_CHECK_STRING_EQUAL(s1, "plain");
    TEST_CHECK_STRING_EQUAL(s2, "plain text");
    TEST_CHECK_STRING_EQUAL(s3, "plain%text");

    sentry_free(s1);
    sentry_free(s2);
    sentry_free(s3);

    // Escape sequences
    char *s4 = sentry__metrics_sanitize_tag_value("plain \\ text");
    char *s5 = sentry__metrics_sanitize_tag_value("plain,text");
    char *s6 = sentry__metrics_sanitize_tag_value("plain|text");

    TEST_CHECK_STRING_EQUAL(s4, "plain \\\\ text");
    TEST_CHECK_STRING_EQUAL(s5, "plain\\u{2c}text");
    TEST_CHECK_STRING_EQUAL(s6, "plain\\u{7c}text");

    sentry_free(s4);
    sentry_free(s5);
    sentry_free(s6);

    // Escapable control characters
    char *s7 = sentry__metrics_sanitize_tag_value("plain\ntext");
    char *s8 = sentry__metrics_sanitize_tag_value("plain\rtext");
    char *s9 = sentry__metrics_sanitize_tag_value("plain\ttext");

    TEST_CHECK_STRING_EQUAL(s7, "plain\\ntext");
    TEST_CHECK_STRING_EQUAL(s8, "plain\\rtext");
    TEST_CHECK_STRING_EQUAL(s9, "plain\\ttext");

    sentry_free(s7);
    sentry_free(s8);
    sentry_free(s9);
}

