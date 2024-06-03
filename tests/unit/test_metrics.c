#include "sentry_metrics.h"
#include "sentry_sync.h"
#include "sentry_testsupport.h"

#define TEST_CHECK_METRICS_SANITY(fn, input, expected)                         \
    {                                                                          \
        char *actual = fn(input);                                              \
        TEST_CHECK_STRING_EQUAL(actual, expected);                             \
        sentry_free(actual);                                                   \
    }

sentry_value_t
get_bucket_item(const sentry_metrics_aggregator_t *aggregator)
{
    TEST_CHECK(sentry_value_get_length(aggregator->buckets) == 1);
    sentry_value_t bucket = sentry_value_get_by_index(aggregator->buckets, 0);
    TEST_CHECK(!sentry_value_is_null(bucket));
    sentry_value_t bucket_metrics = sentry_value_get_by_key(bucket, "metrics");
    TEST_CHECK(!sentry_value_is_null(bucket_metrics));
    TEST_CHECK(sentry_value_get_length(bucket_metrics) == 1);
    return sentry_value_get_by_index(bucket_metrics, 0);
}

SENTRY_TEST(metrics_new_counter)
{
    sentry_metrics_emit_increment(
        "counter_metric", 1.0, "second", "key1", "val1");

    SENTRY_WITH_METRICS_AGGREGATOR(aggregator)
    {
        sentry_value_t bucket_item = get_bucket_item(aggregator);
        TEST_CHECK(!sentry_value_is_null(bucket_item));
        const char *item_key = sentry_value_as_string(
            sentry_value_get_by_key(bucket_item, "key"));
        TEST_CHECK_STRING_EQUAL(item_key, "c_counter_metric_second_key1=val1");
        sentry_value_t metric = sentry_value_get_by_key(bucket_item, "metric");
        TEST_CHECK(!sentry_value_is_null(metric));
        const char *key
            = sentry_value_as_string(sentry_value_get_by_key(metric, "key"));
        TEST_CHECK_STRING_EQUAL(key, "counter_metric");
        const char *timestamp = sentry_value_as_string(
            sentry_value_get_by_key(metric, "timestamp"));
        TEST_CHECK_STRING_EQUAL(timestamp, "2024-05-28T00:00:10Z");
        const char *type
            = sentry_value_as_string(sentry_value_get_by_key(metric, "type"));
        TEST_CHECK_STRING_EQUAL(type, "counter");
        const char *unit
            = sentry_value_as_string(sentry_value_get_by_key(metric, "unit"));
        TEST_CHECK_STRING_EQUAL(unit, "second");
        sentry_value_t value = sentry_value_get_by_key(metric, "value");
        TEST_CHECK(sentry_value_get_type(value) == SENTRY_VALUE_TYPE_DOUBLE);
        TEST_CHECK(sentry_value_as_double(value) == 1.0);
        sentry_value_t tags = sentry_value_get_by_key(metric, "tags");
        sentry_value_t tag_item = sentry_value_get_by_index(tags, 0);
        const char *tag_key
            = sentry_value_as_string(sentry_value_get_by_key(tag_item, "key"));
        TEST_CHECK_STRING_EQUAL(tag_key, "key1");
        const char *tag_val = sentry_value_as_string(
            sentry_value_get_by_key(tag_item, "value"));
        TEST_CHECK_STRING_EQUAL(tag_val, "val1");
    }

    sentry__metrics_aggregator_cleanup();
}

SENTRY_TEST(metrics_new_distribution)
{
    sentry_metrics_emit_distribution("distribution_metric", 1.0, "second");

    SENTRY_WITH_METRICS_AGGREGATOR(aggregator)
    {
        sentry_value_t bucket_item = get_bucket_item(aggregator);
        TEST_CHECK(!sentry_value_is_null(bucket_item));
        const char *item_key = sentry_value_as_string(
            sentry_value_get_by_key(bucket_item, "key"));
        TEST_CHECK_STRING_EQUAL(item_key, "d_distribution_metric_second_");
        sentry_value_t metric = sentry_value_get_by_key(bucket_item, "metric");
        TEST_CHECK(!sentry_value_is_null(metric));
        const char *key
            = sentry_value_as_string(sentry_value_get_by_key(metric, "key"));
        TEST_CHECK_STRING_EQUAL(key, "distribution_metric");
        const char *timestamp = sentry_value_as_string(
            sentry_value_get_by_key(metric, "timestamp"));
        TEST_CHECK_STRING_EQUAL(timestamp, "2024-05-28T00:00:10Z");
        const char *type
            = sentry_value_as_string(sentry_value_get_by_key(metric, "type"));
        TEST_CHECK_STRING_EQUAL(type, "distribution");
        sentry_value_t value = sentry_value_get_by_key(metric, "value");
        TEST_CHECK(sentry_value_get_type(value) == SENTRY_VALUE_TYPE_LIST);
        TEST_CHECK(sentry_value_get_length(value) == 1);
        const double dist_item
            = sentry_value_as_double(sentry_value_get_by_index(value, 0));
        TEST_CHECK(dist_item == 1.0);
    }

    sentry__metrics_aggregator_cleanup();
}

SENTRY_TEST(metrics_new_gauge)
{
    sentry_metrics_emit_gauge("gauge_metric", 1.0, "second");

    SENTRY_WITH_METRICS_AGGREGATOR(aggregator)
    {
        sentry_value_t bucket_item = get_bucket_item(aggregator);
        TEST_CHECK(!sentry_value_is_null(bucket_item));
        const char *item_key = sentry_value_as_string(
            sentry_value_get_by_key(bucket_item, "key"));
        TEST_CHECK_STRING_EQUAL(item_key, "g_gauge_metric_second_");
        sentry_value_t metric = sentry_value_get_by_key(bucket_item, "metric");
        TEST_CHECK(!sentry_value_is_null(metric));
        const char *key
            = sentry_value_as_string(sentry_value_get_by_key(metric, "key"));
        TEST_CHECK_STRING_EQUAL(key, "gauge_metric");
        const char *type
            = sentry_value_as_string(sentry_value_get_by_key(metric, "type"));
        TEST_CHECK_STRING_EQUAL(type, "gauge");
        const char *timestamp = sentry_value_as_string(
            sentry_value_get_by_key(metric, "timestamp"));
        TEST_CHECK_STRING_EQUAL(timestamp, "2024-05-28T00:00:10Z");
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
    }

    sentry__metrics_aggregator_cleanup();
}

SENTRY_TEST(metrics_new_set)
{
    sentry_metrics_emit_set("set_metric", 1.0, "second");

    SENTRY_WITH_METRICS_AGGREGATOR(aggregator)
    {
        sentry_value_t bucket_item = get_bucket_item(aggregator);
        TEST_CHECK(!sentry_value_is_null(bucket_item));
        const char *item_key = sentry_value_as_string(
            sentry_value_get_by_key(bucket_item, "key"));
        TEST_CHECK_STRING_EQUAL(item_key, "s_set_metric_second_");
        sentry_value_t metric = sentry_value_get_by_key(bucket_item, "metric");
        TEST_CHECK(!sentry_value_is_null(metric));
        const char *key
            = sentry_value_as_string(sentry_value_get_by_key(metric, "key"));
        TEST_CHECK_STRING_EQUAL(key, "set_metric");
        const char *type
            = sentry_value_as_string(sentry_value_get_by_key(metric, "type"));
        TEST_CHECK_STRING_EQUAL(type, "set");
        const char *timestamp = sentry_value_as_string(
            sentry_value_get_by_key(metric, "timestamp"));
        TEST_CHECK_STRING_EQUAL(timestamp, "2024-05-28T00:00:10Z");
        sentry_value_t value = sentry_value_get_by_key(metric, "value");
        TEST_CHECK(sentry_value_get_type(value) == SENTRY_VALUE_TYPE_LIST);
        TEST_CHECK(sentry_value_get_length(value) == 1);
        const double set_item
            = sentry_value_as_int32(sentry_value_get_by_index(value, 0));
        TEST_CHECK(set_item == 1);
    }

    sentry__metrics_aggregator_cleanup();
}

SENTRY_TEST(metrics_bucket_weight)
{
    sentry_metrics_emit_increment("counter1", 1.0, "second");
    sentry_metrics_emit_increment("counter2", 1.0, "second");
    sentry_metrics_emit_distribution("distribution", 1.0, "second");
    sentry_metrics_emit_distribution("distribution", 1.0, "second");
    sentry_metrics_emit_distribution("distribution", 2.0, "second");
    sentry_metrics_emit_gauge("gauge", 1.0, "second");
    sentry_metrics_emit_set("set", 1, "second");
    sentry_metrics_emit_set("set", 1, "second");
    sentry_metrics_emit_set("set", 2, "second");

    SENTRY_WITH_METRICS_AGGREGATOR(aggregator)
    {
        TEST_CHECK(sentry_value_get_length(aggregator->buckets) == 1);
        sentry_value_t bucket
            = sentry_value_get_by_index(aggregator->buckets, 0);
        TEST_CHECK(!sentry_value_is_null(bucket));

        TEST_CHECK_INT_EQUAL(sentry__metrics_get_bucket_weight(bucket), 12);
    }

    sentry__metrics_aggregator_cleanup();
}

SENTRY_TEST(metrics_add)
{
    sentry_metrics_emit_increment("counter", 1.0, "second");
    sentry_metrics_emit_increment("counter", 2.0, "second");
    sentry_metrics_emit_increment("counter", 3.0, "second");
    SENTRY_WITH_METRICS_AGGREGATOR(aggregator)
    {
        sentry_value_t bucket_item = get_bucket_item(aggregator);
        sentry_value_t metric = sentry_value_get_by_key(bucket_item, "metric");
        sentry_value_t metric_value = sentry_value_get_by_key(metric, "value");
        TEST_CHECK(sentry_value_as_double(metric_value) == 6.0);
    }
    sentry__metrics_aggregator_cleanup();

    sentry_metrics_emit_distribution("dist", 1.0, "second");
    sentry_metrics_emit_distribution("dist", 2.0, "second");
    sentry_metrics_emit_distribution("dist", 3.0, "second");
    SENTRY_WITH_METRICS_AGGREGATOR(aggregator)
    {
        sentry_value_t bucket_item = get_bucket_item(aggregator);
        sentry_value_t metric = sentry_value_get_by_key(bucket_item, "metric");
        sentry_value_t metric_value = sentry_value_get_by_key(metric, "value");

        TEST_CHECK(sentry_value_get_length(metric_value) == 3);
        TEST_CHECK(
            sentry_value_as_double(sentry_value_get_by_index(metric_value, 0))
            == 1.0);
        TEST_CHECK(
            sentry_value_as_double(sentry_value_get_by_index(metric_value, 1))
            == 2.0);
        TEST_CHECK(
            sentry_value_as_double(sentry_value_get_by_index(metric_value, 2))
            == 3.0);
    }
    sentry__metrics_aggregator_cleanup();

    sentry_metrics_emit_gauge("gauge", 1.0, "second");
    sentry_metrics_emit_gauge("gauge", 2.0, "second");
    sentry_metrics_emit_gauge("gauge", 3.0, "second");
    SENTRY_WITH_METRICS_AGGREGATOR(aggregator)
    {
        sentry_value_t bucket_item = get_bucket_item(aggregator);
        sentry_value_t metric = sentry_value_get_by_key(bucket_item, "metric");
        sentry_value_t metric_value = sentry_value_get_by_key(metric, "value");

        sentry_value_t last = sentry_value_get_by_key(metric_value, "last");
        TEST_CHECK(sentry_value_as_double(last) == 3.0);
        sentry_value_t min = sentry_value_get_by_key(metric_value, "min");
        TEST_CHECK(sentry_value_as_double(min) == 1.0);
        sentry_value_t max = sentry_value_get_by_key(metric_value, "max");
        TEST_CHECK(sentry_value_as_double(max) == 3.0);
        sentry_value_t sum = sentry_value_get_by_key(metric_value, "sum");
        TEST_CHECK(sentry_value_as_double(sum) == 6.0);
        sentry_value_t count = sentry_value_get_by_key(metric_value, "count");
        TEST_CHECK(sentry_value_as_int32(count) == 3);
    }
    sentry__metrics_aggregator_cleanup();

    sentry_metrics_emit_set("set", 1.0, "second");
    sentry_metrics_emit_set("set", 2.0, "second");
    sentry_metrics_emit_set("set", 2.0, "second");
    SENTRY_WITH_METRICS_AGGREGATOR(aggregator)
    {
        sentry_value_t bucket_item = get_bucket_item(aggregator);
        sentry_value_t metric = sentry_value_get_by_key(bucket_item, "metric");
        sentry_value_t metric_value = sentry_value_get_by_key(metric, "value");

        TEST_CHECK(sentry_value_get_length(metric_value) == 2);
        TEST_CHECK(
            sentry_value_as_double(sentry_value_get_by_index(metric_value, 0))
            == 1.0);
        TEST_CHECK(
            sentry_value_as_double(sentry_value_get_by_index(metric_value, 1))
            == 2.0);
    }
    sentry__metrics_aggregator_cleanup();
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
