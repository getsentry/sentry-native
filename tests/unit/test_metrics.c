#include "sentry_metrics.h"
#include "sentry_testsupport.h"

SENTRY_TEST(metrics_new_counter)
{
    sentry_metric_t *opaque_metric
        = sentry_metrics_new_increment("counter_metric", 1.0);

    sentry_metric_set_unit(opaque_metric, "second");
    sentry_metric_set_tag(opaque_metric, "key1", "val1");

    sentry_value_t metric;
    if (opaque_metric != NULL) {
        metric = opaque_metric->inner;
        TEST_CHECK(!sentry_value_is_null(metric));
        TEST_CHECK(opaque_metric->type == SENTRY_METRIC_COUNTER);
        const char *type
            = sentry_value_as_string(sentry_value_get_by_key(metric, "type"));
        TEST_CHECK_STRING_EQUAL(type, "counter");
        const char *unit
            = sentry_value_as_string(sentry_value_get_by_key(metric, "unit"));
        TEST_CHECK_STRING_EQUAL(unit, "second");
        sentry_value_t value = sentry_value_get_by_key(metric, "value");
        TEST_CHECK(sentry_value_get_type(value) == SENTRY_VALUE_TYPE_DOUBLE);
        TEST_CHECK(sentry_value_as_double(value) == 1.0);
        TEST_CHECK(opaque_metric->value == 1.0);
        sentry_value_t tags = sentry_value_get_by_key(metric, "tags");
        const char *tag_val
            = sentry_value_as_string(sentry_value_get_by_key(tags, "key1"));
        TEST_CHECK_STRING_EQUAL(tag_val, "val1");
    } else {
        TEST_CHECK(opaque_metric != NULL);
    }

    sentry__metric_free(opaque_metric);
}

SENTRY_TEST(metrics_new_distribution)
{
    sentry_metric_t *opaque_metric
        = sentry_metrics_new_distribution("distribution_metric", 1.0);

    sentry_value_t metric;
    if (opaque_metric != NULL) {
        metric = opaque_metric->inner;
        TEST_CHECK(!sentry_value_is_null(metric));
        TEST_CHECK(opaque_metric->type == SENTRY_METRIC_DISTRIBUTION);
        const char *type
            = sentry_value_as_string(sentry_value_get_by_key(metric, "type"));
        TEST_CHECK_STRING_EQUAL(type, "distribution");
        sentry_value_t value = sentry_value_get_by_key(metric, "value");
        TEST_CHECK(sentry_value_get_type(value) == SENTRY_VALUE_TYPE_LIST);
        TEST_CHECK(sentry_value_get_length(value) == 1);
        const double dist_item
            = sentry_value_as_double(sentry_value_get_by_index(value, 0));
        TEST_CHECK(dist_item == 1.0);
        TEST_CHECK(opaque_metric->value == 1.0);
    } else {
        TEST_CHECK(opaque_metric != NULL);
    }

    sentry__metric_free(opaque_metric);
}

SENTRY_TEST(metrics_new_gauge)
{
    sentry_metric_t *opaque_metric
        = sentry_metrics_new_gauge("gauge_metric", 1.0);

    sentry_value_t metric;
    if (opaque_metric != NULL) {
        metric = opaque_metric->inner;
        TEST_CHECK(!sentry_value_is_null(metric));
        TEST_CHECK(opaque_metric->type == SENTRY_METRIC_GAUGE);
        const char *type
            = sentry_value_as_string(sentry_value_get_by_key(metric, "type"));
        TEST_CHECK_STRING_EQUAL(type, "gauge");
        sentry_value_t value = sentry_value_get_by_key(metric, "value");
        TEST_CHECK(sentry_value_get_type(value) == SENTRY_VALUE_TYPE_OBJECT);
        const double last
            = sentry_value_as_double(sentry_value_get_by_key(value, "last"));
        TEST_CHECK(last == 1.0);
        const double min
            = sentry_value_as_double(sentry_value_get_by_key(value, "min"));
        TEST_CHECK(min == 1.0);
        const double max
            = sentry_value_as_double(sentry_value_get_by_key(value, "max"));
        TEST_CHECK(max == 1.0);
        const double sum
            = sentry_value_as_double(sentry_value_get_by_key(value, "sum"));
        TEST_CHECK(sum == 1.0);
        const int32_t count
            = sentry_value_as_int32(sentry_value_get_by_key(value, "count"));
        TEST_CHECK(count == 1.0);
        TEST_CHECK(opaque_metric->value == 1.0);
    } else {
        TEST_CHECK(opaque_metric != NULL);
    }

    sentry__metric_free(opaque_metric);
}

SENTRY_TEST(metrics_new_set)
{
    sentry_metric_t *opaque_metric
        = sentry_metrics_new_set("set_metric", 1);

    sentry_value_t metric;
    if (opaque_metric != NULL) {
        metric = opaque_metric->inner;
        TEST_CHECK(!sentry_value_is_null(metric));
        TEST_CHECK(opaque_metric->type == SENTRY_METRIC_SET);
        const char *type
            = sentry_value_as_string(sentry_value_get_by_key(metric, "type"));
        TEST_CHECK_STRING_EQUAL(type, "set");
        sentry_value_t value = sentry_value_get_by_key(metric, "value");
        TEST_CHECK(sentry_value_get_type(value) == SENTRY_VALUE_TYPE_LIST);
        TEST_CHECK(sentry_value_get_length(value) == 1);
        const double set_item
            = sentry_value_as_int32(sentry_value_get_by_index(value, 0));
        TEST_CHECK(set_item == 1);
        TEST_CHECK((int32_t)opaque_metric->value == 1);
    } else {
        TEST_CHECK(opaque_metric != NULL);
    }

    sentry__metric_free(opaque_metric);
}

SENTRY_TEST(metrics_name_sanitize)
{
    char *name1 = sentry__metrics_sanitize_name("foo-bar");
    char *name2 = sentry__metrics_sanitize_name("foo\\!,bar");
    char *name3 = sentry__metrics_sanitize_name("fo%-bar");

    TEST_CHECK_STRING_EQUAL(name1, "foo-bar");
    TEST_CHECK_STRING_EQUAL(name2, "foo\\__bar");
    TEST_CHECK_STRING_EQUAL(name3, "fo_-bar");

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
