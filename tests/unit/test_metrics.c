#include "sentry_metrics.h"
#include "sentry_testsupport.h"

SENTRY_TEST(metrics_name_sanitize)
{
    char *name1 = sentry__metrics_sanitize_name("foo-bar");
    char *name2 = sentry__metrics_sanitize_name("foo\\!,bar");
    char *name3 = sentry__metrics_sanitize_name("fo%-bar");
    //char *name4 = sentry__metrics_sanitize_name("fôo-bär");

    TEST_CHECK_STRING_EQUAL(name1, "foo-bar");
    TEST_CHECK_STRING_EQUAL(name2, "foo\\__bar");
    TEST_CHECK_STRING_EQUAL(name3, "fo_-bar");
    //TEST_CHECK_STRING_EQUAL(name4, "f_o-b_r");

    sentry_free(name1);
    sentry_free(name2);
    sentry_free(name3);
}

SENTRY_TEST(metrics_unit_sanitize)
{
    char *unit = sentry__metrics_sanitize_unit("abcABC123_-./\\/%&abcABC12");

    TEST_CHECK_STRING_EQUAL(unit, "abcABC123_abcABC12");

    sentry_free(unit);
}

SENTRY_TEST(metrics_tag_key_sanitize)
{
    char *key = sentry__metrics_sanitize_tag_key("a/weird/tag-key/:\\$");

    TEST_CHECK_STRING_EQUAL(key, "a/weird/tag-key/\\");

    sentry_free(key);
}

SENTRY_TEST(metrics_tag_value_sanitize)
{
    char *val1 = sentry__metrics_sanitize_tag_value("plain");
    char *val2 = sentry__metrics_sanitize_tag_value("plain text");
    char *val3 = sentry__metrics_sanitize_tag_value("plain%text");

    TEST_CHECK_STRING_EQUAL(val1, "plain");
    TEST_CHECK_STRING_EQUAL(val2, "plain text");
    TEST_CHECK_STRING_EQUAL(val3, "plain%text");

    sentry_free(val1);
    sentry_free(val2);
    sentry_free(val3);

    // Escape sequences
    char *val4 = sentry__metrics_sanitize_tag_value("plain \\ text");
    char *val5 = sentry__metrics_sanitize_tag_value("plain,text");
    char *val6 = sentry__metrics_sanitize_tag_value("plain|text");

    TEST_CHECK_STRING_EQUAL(val4, "plain \\\\ text");
    TEST_CHECK_STRING_EQUAL(val5, "plain\\u{2c}text");
    TEST_CHECK_STRING_EQUAL(val6, "plain\\u{7c}text");

    sentry_free(val4);
    sentry_free(val5);
    sentry_free(val6);

    // Escapable control characters
    char *val7 = sentry__metrics_sanitize_tag_value("plain\ntext");
    char *val8 = sentry__metrics_sanitize_tag_value("plain\rtext");
    char *val9 = sentry__metrics_sanitize_tag_value("plain\ttext");

    TEST_CHECK_STRING_EQUAL(val7, "plain\\ntext");
    TEST_CHECK_STRING_EQUAL(val8, "plain\\rtext");
    TEST_CHECK_STRING_EQUAL(val9, "plain\\ttext");

    sentry_free(val7);
    sentry_free(val8);
    sentry_free(val9);
}
