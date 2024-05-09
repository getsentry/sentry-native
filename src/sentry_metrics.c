#include "sentry_metrics.h"
#include "sentry.h"
#include "sentry_alloc.h"
#include "sentry_random.h"
#include "sentry_slice.h"
#include "sentry_string.h"
#include "sentry_sync.h"
#include "sentry_utils.h"

#include <stdlib.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define ROLLUP_IN_SECONDS 10



static bool g_aggregator_initialized = false;
static sentry_metrics_aggregator_t g_aggregator = { 0 };
static sentry_mutex_t g_aggregator_lock = SENTRY__MUTEX_INIT;

sentry_value_t
sentry__metrics_type_to_string(sentry_metric_type_t type)
{
    switch (type) {
    case SENTRY_METRIC_COUNTER:
        return sentry_value_new_string("counter");
    case SENTRY_METRIC_DISTRIBUTION:
        return sentry_value_new_string("distribution");
    case SENTRY_METRIC_GAUGE:
        return sentry_value_new_string("gauge");
    case SENTRY_METRIC_SET:
        return sentry_value_new_string("set");
    default:
        assert(!"invalid metric type");
        return sentry_value_new_null();
    }
}

const char *
sentry__metrics_type_to_statsd(sentry_metric_type_t type)
{
    switch (type) {
    case SENTRY_METRIC_COUNTER:
        return "c";
    case SENTRY_METRIC_DISTRIBUTION:
        return "d";
    case SENTRY_METRIC_GAUGE:
        return "g";
    case SENTRY_METRIC_SET:
        return "s";
    default:
        assert(!"invalid metric type");
        return "invalid";
    }
}

void
sentry__metrics_type_from_string(
    sentry_value_t type, sentry_metric_type_t *metric_type)
{
    const char *typeStr = sentry_value_as_string(type);

    switch (*typeStr) {
    case 'c':
        *metric_type = SENTRY_METRIC_COUNTER;
        break;
    case 'd':
        *metric_type = SENTRY_METRIC_DISTRIBUTION;
        break;
    case 'g':
        *metric_type = SENTRY_METRIC_GAUGE;
        break;
    case 's':
        *metric_type = SENTRY_METRIC_SET;
        break;
    default:
        assert(!"invalid metric type");
    }
}

int has_name_pattern_match(char c) {
    return isalnum(c) || c == '_' || c == '\\' || c == '-' || c == '.';
}

int has_unit_pattern_match(char c) {
    return isalnum(c) || c == '_';
}

int has_tag_key_pattern_match(char c) {
    return isalnum(c) || c == '_' || c == '\\' || c == '-' || c == '.'
        || c == '/';
}

char *
sentry__metrics_sanitize(const char *original, const char *replacement,
    int (*pattern_match_func)(char c))
{
    sentry_stringbuilder_t sb;
    sentry__stringbuilder_init(&sb);

    const unsigned char *ptr = (const unsigned char *)original;
    for (; *ptr; ptr++) {
        if (pattern_match_func(*ptr)) {
            sentry__stringbuilder_append_char(&sb, *ptr);
        } else {
            sentry__stringbuilder_append(&sb, replacement);
        }
    }

    return sentry__stringbuilder_into_string(&sb);
}

char *
sentry__metrics_sanitize_name(const char *name)
{
    return sentry__metrics_sanitize(
        name, "_", has_tag_key_pattern_match);
}

char *
sentry__metrics_sanitize_unit(const char *unit)
{
    return sentry__metrics_sanitize(
        unit, "", has_tag_key_pattern_match);
}

char *
sentry__metrics_sanitize_tag_key(const char *tag_value)
{
    return sentry__metrics_sanitize(
        tag_value, "", has_tag_key_pattern_match);
}

char *
sentry__metrics_sanitize_tag_value(const char *tag_value)
{
    sentry_stringbuilder_t sb;
    sentry__stringbuilder_init(&sb);

    const unsigned char *ptr = (const unsigned char *)tag_value;
    for (; *ptr; ptr++) {
        switch (*ptr) {
        case '\n':
            sentry__stringbuilder_append(&sb, "\\n");
            break;
        case '\r':
            sentry__stringbuilder_append(&sb, "\\r");
            break;
        case '\t':
            sentry__stringbuilder_append(&sb, "\\t");
            break;
        case '\\':
            sentry__stringbuilder_append(&sb, "\\\\");
            break;
        case '|':
            sentry__stringbuilder_append(&sb, "\\u{7c}");
            break;
        case ',':
            sentry__stringbuilder_append(&sb, "\\u{2c}");
            break;
        default:
            sentry__stringbuilder_append_char(&sb, *ptr);
        }
    }

    return sentry__stringbuilder_into_string(&sb);
}

static sentry_metrics_aggregator_t *
get_metrics_aggregator(void)
{
    if (g_aggregator_initialized) {
        return &g_aggregator;
    }

    memset(&g_aggregator, 0, sizeof(sentry_metrics_aggregator_t));
    g_aggregator.buckets = sentry_value_new_list();

    g_aggregator_initialized = true;

    return &g_aggregator;
}

sentry_metrics_aggregator_t *
sentry__metrics_aggregator_lock(void)
{
    sentry__mutex_lock(&g_aggregator_lock);
    return get_metrics_aggregator();
}

void
sentry__metrics_aggregator_unlock(void)
{
    sentry__mutex_unlock(&g_aggregator_lock);
}

void
sentry__metrics_aggregator_cleanup(void)
{
    sentry__mutex_lock(&g_aggregator_lock);
    if (g_aggregator_initialized) {
        g_aggregator_initialized = false;
        sentry_value_decref(g_aggregator.buckets);
    }
    sentry__mutex_unlock(&g_aggregator_lock);
}

sentry_value_t
sentry__metrics_get_bucket_key(uint64_t timestamp)
{
    uint64_t seconds = timestamp / 1000;
    uint64_t bucketKey = (seconds / ROLLUP_IN_SECONDS) * ROLLUP_IN_SECONDS;

    char buf[20 + 1];
    size_t written = (size_t)snprintf(
        buf, sizeof(buf), "%llu", (unsigned long long)bucketKey);
    if (written >= sizeof(buf)) {
        return sentry_value_new_null();
    }
    buf[written] = '\0';
    return sentry_value_new_string(buf);
}

uint64_t
sentry__metrics_get_cutoff_timestamp(uint64_t timestamp)
{
    uint64_t rnd;
    sentry__getrandom(&rnd, sizeof(rnd));

    uint64_t flushShiftMs = (uint64_t)((double)rnd / (double)UINT64_MAX
        * ROLLUP_IN_SECONDS * 1000);

    uint64_t cutoffTimestamp
        = timestamp - ROLLUP_IN_SECONDS * 1000 - flushShiftMs;

    uint64_t seconds = cutoffTimestamp / 1000;

    return (seconds / ROLLUP_IN_SECONDS) * ROLLUP_IN_SECONDS;
}

uint64_t
sentry__metrics_timestamp_from_string(const char *timestampStr)
{
    char *endptr;
    uint64_t timestamp = strtoull(timestampStr, &endptr, 10);

    if (*endptr != '\0') {
        SENTRY_WARN("invalid timestamp");
        return 0;
    }

    return timestamp;
}

sentry_value_t
sentry__metrics_get_or_add_bucket(
    const sentry_metrics_aggregator_t *aggregator, sentry_value_t bucketKey)
{
    size_t len = sentry_value_get_length(aggregator->buckets);
    for (size_t i = 0; i < len; i++) {
        sentry_value_t existingBucket
            = sentry_value_get_by_index(aggregator->buckets, i);
        const char *key = sentry_value_as_string(
            sentry_value_get_by_key(existingBucket, "key"));
        if (sentry__string_eq(key, sentry_value_as_string(bucketKey))) {
            return existingBucket;
        }
    }

    // if there is no existing bucket with given key
    // create a new one and add it to aggregator
    sentry_value_t newBucket = sentry_value_new_object();
    sentry_value_set_by_key(newBucket, "key", bucketKey);
    sentry_value_set_by_key(newBucket, "metrics", sentry_value_new_list());
    sentry_value_append(aggregator->buckets, newBucket);
    return newBucket;
}

char *
sentry__metrics_get_tags_key(sentry_value_t tags)
{
    sentry_stringbuilder_t sb;
    sentry__stringbuilder_init(&sb);

    for (size_t i = 0; i < sentry_value_get_length(tags); i++) {
        sentry_value_t tagItem = sentry_value_get_by_index(tags, i);
        if (sentry__stringbuilder_len(&sb) > 0) {
            sentry__stringbuilder_append(&sb, ",");
        }
        sentry__stringbuilder_append(&sb,
            sentry_value_as_string(sentry_value_get_by_key(tagItem, "key")));
        sentry__stringbuilder_append_char(&sb, '=');
        sentry__stringbuilder_append(&sb,
            sentry_value_as_string(sentry_value_get_by_key(tagItem, "value")));
    }

    return sentry__stringbuilder_into_string(&sb);
}

char *
sentry__metrics_get_metric_bucket_key(sentry_metric_t *metric)
{
    const char *typePrefix = sentry__metrics_type_to_statsd(metric->type);

    const char *metricKey
        = sentry_value_as_string(sentry_value_get_by_key(metric->inner, "key"));

    const char *unitName = sentry_value_as_string(
        sentry_value_get_by_key(metric->inner, "unit"));

    const char *serializedTags = sentry__metrics_get_tags_key(
        sentry_value_get_by_key(metric->inner, "tags"));

    size_t keyLength = strlen(typePrefix) + strlen(metricKey) + strlen(unitName)
        + strlen(serializedTags) + 4;
    char *metricBucketKey = sentry_malloc(keyLength);
    size_t written = snprintf(metricBucketKey, keyLength, "%s_%s_%s_%s",
        typePrefix, metricKey, unitName, serializedTags);
    metricBucketKey[written] = '\0';

    return metricBucketKey;
}

sentry_value_t
sentry__metrics_find_in_bucket(sentry_value_t bucket, const char *metricKey)
{
    sentry_value_t metrics = sentry_value_get_by_key(bucket, "metrics");
    size_t len = sentry_value_get_length(metrics);
    for (size_t i = 0; i < len; i++) {
        sentry_value_t existingMetric = sentry_value_get_by_index(metrics, i);
        const char *key = sentry_value_as_string(
            sentry_value_get_by_key(existingMetric, "key"));
        if (sentry__string_eq(key, metricKey)) {
            return sentry_value_get_by_key(existingMetric, "metric");
        }
    }

    return sentry_value_new_null();
}

void
sentry__metrics_aggregator_add(
    const sentry_metrics_aggregator_t *aggregator, sentry_metric_t *metric)
{
    uint64_t timestamp = sentry__iso8601_to_msec(sentry_value_as_string(
        sentry_value_get_by_key(metric->inner, "timestamp")));

    sentry_value_t bucketKey = sentry__metrics_get_bucket_key(timestamp);

    sentry_value_t bucket
        = sentry__metrics_get_or_add_bucket(aggregator, bucketKey);

    char *metricBucketKey = sentry__metrics_get_metric_bucket_key(metric);

    sentry_value_t existingMetric
        = sentry__metrics_find_in_bucket(bucket, metricBucketKey);

    if (sentry_value_is_null(existingMetric)) {
        sentry_value_t newBucketItem = sentry_value_new_object();
        sentry_value_set_by_key(
            newBucketItem, "key", sentry_value_new_string(metricBucketKey));
        sentry_value_set_by_key(newBucketItem, "metric", metric->inner);
        sentry_value_append(
            sentry_value_get_by_key(bucket, "metrics"), newBucketItem);
    } else {
        switch (metric->type) {
        case SENTRY_METRIC_COUNTER:
            sentry__metrics_increment_add(existingMetric, metric->value);
            break;
        case SENTRY_METRIC_DISTRIBUTION:
            sentry__metrics_distribution_add(existingMetric, metric->value);
            break;
        case SENTRY_METRIC_GAUGE:
            sentry__metrics_gauge_add(existingMetric, metric->value);
            break;
        case SENTRY_METRIC_SET:
            sentry__metrics_set_add(existingMetric, metric->value);
            break;
        default:
            SENTRY_WARN("uknown metric type");
        }
    }

    sentry_free(metricBucketKey);
}

void
sentry__metrics_aggregator_flush(
    const sentry_metrics_aggregator_t *aggregator, bool force)
{
    if (force) {
        // TODO: if true all aggregated buckets should be flushed
        return;
    }

    sentry_value_t flushableBuckets = sentry_value_new_list();

    uint64_t cutoffTimestamp
        = sentry__metrics_get_cutoff_timestamp(sentry__msec_time());

    size_t bucketsLen = sentry_value_get_length(aggregator->buckets);

    // Since size_t is unsigned decrementing i after it is zero
    // yields the largest size_t value and the loop works correctly.
    for (size_t i = bucketsLen - 1; i < bucketsLen; i--) {
        sentry_value_t bucket
            = sentry_value_get_by_index_owned(aggregator->buckets, i);

        uint64_t bucketTimestamp = sentry__metrics_timestamp_from_string(
            sentry_value_as_string(sentry_value_get_by_key(bucket, "key")));

        if (bucketTimestamp < cutoffTimestamp) {
            sentry_value_append(flushableBuckets, bucket);
            sentry_value_remove_by_index(aggregator->buckets, i);
        }
    }

    sentry__metrics_flush(sentry__metrics_encode_statsd(flushableBuckets));

    sentry_value_decref(flushableBuckets);
}

void
sentry__metric_free(sentry_metric_t *metric)
{
    if (!metric) {
        return;
    }
    if (sentry_value_refcount(metric->inner) <= 1) {
        sentry_value_decref(metric->inner);
        sentry_free(metric);
    } else {
        sentry_value_decref(metric->inner);
    }
}

sentry_value_t
sentry__value_metric_new_n(sentry_slice_t name)
{
    sentry_value_t metric = sentry_value_new_object();

    sentry_value_set_by_key(
        metric, "key", sentry_value_new_string_n(name.ptr, name.len));

    sentry_value_set_by_key(metric, "timestamp",
        sentry__value_new_string_owned(
            sentry__msec_time_to_iso8601(sentry__msec_time())));

    sentry_value_set_by_key(metric, "unit", sentry_value_new_string("none"));

    return metric;
}

sentry_metric_t *
sentry_metrics_new_increment_n(const char *key, size_t key_len, double value)
{
    sentry_metric_t *metric = SENTRY_MAKE(sentry_metric_t);
    if (!metric) {
        return NULL;
    }

    metric->type = SENTRY_METRIC_COUNTER;
    metric->inner
        = sentry__value_metric_new_n((sentry_slice_t) { key, key_len });
    sentry_value_set_by_key(
        metric->inner, "type", sentry__metrics_type_to_string(metric->type));

    metric->value = sentry_value_new_double(value);

    sentry_value_set_by_key(
        metric->inner, "value", sentry_value_new_double(value));

    if (sentry_value_is_null(metric->inner)) {
        sentry_free(metric);
        return NULL;
    }

    return metric;
}

sentry_metric_t *
sentry_metrics_new_increment(const char *key, double value)
{
    size_t key_len = key ? strlen(key) : 0;

    return sentry_metrics_new_increment_n(key, key_len, value);
}

sentry_metric_t *
sentry_metrics_new_distribution_n(const char *key, size_t key_len, double value)
{
    sentry_metric_t *metric = SENTRY_MAKE(sentry_metric_t);
    if (!metric) {
        return NULL;
    }

    metric->type = SENTRY_METRIC_DISTRIBUTION;
    metric->inner
        = sentry__value_metric_new_n((sentry_slice_t) { key, key_len });
    sentry_value_set_by_key(
        metric->inner, "type", sentry__metrics_type_to_string(metric->type));

    metric->value = sentry_value_new_double(value);

    sentry_value_t distributionValues = sentry_value_new_list();
    sentry_value_append(distributionValues, sentry_value_new_double(value));

    sentry_value_set_by_key(metric->inner, "value", distributionValues);

    if (sentry_value_is_null(metric->inner)) {
        sentry_free(metric);
        return NULL;
    }

    return metric;
}

sentry_metric_t *
sentry_metrics_new_distribution(const char *key, double value)
{
    size_t key_len = key ? strlen(key) : 0;

    return sentry_metrics_new_distribution_n(key, key_len, value);
}

sentry_metric_t *
sentry_metrics_new_gauge_n(const char *key, size_t key_len, double value)
{
    sentry_metric_t *metric = SENTRY_MAKE(sentry_metric_t);
    if (!metric) {
        return NULL;
    }

    metric->type = SENTRY_METRIC_GAUGE;
    metric->inner
        = sentry__value_metric_new_n((sentry_slice_t) { key, key_len });
    sentry_value_set_by_key(
        metric->inner, "type", sentry__metrics_type_to_string(metric->type));

    metric->value = sentry_value_new_double(value);

    sentry_value_t gaugeValue = sentry_value_new_object();
    sentry_value_set_by_key(gaugeValue, "last", sentry_value_new_double(value));
    sentry_value_set_by_key(gaugeValue, "min", sentry_value_new_double(value));
    sentry_value_set_by_key(gaugeValue, "max", sentry_value_new_double(value));
    sentry_value_set_by_key(gaugeValue, "sum", sentry_value_new_double(value));
    sentry_value_set_by_key(gaugeValue, "count", sentry_value_new_int32(1));

    sentry_value_set_by_key(metric->inner, "value", gaugeValue);

    if (sentry_value_is_null(metric->inner)) {
        sentry_free(metric);
        return NULL;
    }

    return metric;
}

sentry_metric_t *
sentry_metrics_new_gauge(const char *key, double value)
{
    size_t key_len = key ? strlen(key) : 0;

    return sentry_metrics_new_gauge_n(key, key_len, value);
}

sentry_metric_t *
sentry_metrics_new_set_n(const char *key, size_t key_len, int32_t value)
{
    sentry_metric_t *metric = SENTRY_MAKE(sentry_metric_t);
    if (!metric) {
        return NULL;
    }

    metric->type = SENTRY_METRIC_SET;
    metric->inner
        = sentry__value_metric_new_n((sentry_slice_t) { key, key_len });
    sentry_value_set_by_key(
        metric->inner, "type", sentry__metrics_type_to_string(metric->type));

    metric->value = sentry_value_new_int32(value);

    sentry_value_t setValues = sentry_value_new_list();
    sentry_value_append(setValues, sentry_value_new_int32(value));

    sentry_value_set_by_key(metric->inner, "value", setValues);

    if (sentry_value_is_null(metric->inner)) {
        sentry_free(metric);
        return NULL;
    }

    return metric;
}

sentry_metric_t *
sentry_metrics_new_set(const char *key, int32_t value)
{
    size_t key_len = key ? strlen(key) : 0;

    return sentry_metrics_new_set_n(key, key_len, value);
}

const char *
sentry__metrics_encode_statsd(sentry_value_t buckets)
{
    sentry_stringbuilder_t statsd;
    sentry__stringbuilder_init(&statsd);

    size_t lenBuckets = sentry_value_get_length(buckets);
    for (size_t i = 0; i < lenBuckets; i++) {
        sentry_value_t bucket = sentry_value_get_by_index(buckets, i);
        sentry_value_t metrics = sentry_value_get_by_key(bucket, "metrics");
        for (size_t j = 0; j < sentry_value_get_length(metrics); j++) {
            sentry_value_t metric = sentry_value_get_by_key(
                sentry_value_get_by_index(metrics, j), "metric");

            char *name = sentry__metrics_sanitize_unit(
                sentry_value_as_string(sentry_value_get_by_key(metric, "key")));
            sentry__stringbuilder_append(&statsd, name);
            sentry_free(name);

            sentry__stringbuilder_append(&statsd, "@");

            char *unit = sentry__metrics_sanitize_unit(
                sentry_value_as_string(sentry_value_get_by_key(metric, "unit")));
            sentry__stringbuilder_append(&statsd, unit);
            sentry_free(unit);

            sentry_value_t metricValue
                = sentry_value_get_by_key(metric, "value");

            sentry_metric_type_t metricType;
            sentry__metrics_type_from_string(
                sentry_value_get_by_key(metric, "type"), &metricType);

            switch (metricType) {
            case SENTRY_METRIC_COUNTER:
                sentry__metrics_increment_serialize(&statsd, metricValue);
                break;
            case SENTRY_METRIC_DISTRIBUTION:
                sentry__metrics_distribution_serialize(&statsd, metricValue);
                break;
            case SENTRY_METRIC_GAUGE:
                sentry__metrics_gauge_serialize(&statsd, metricValue);
                break;
            case SENTRY_METRIC_SET:
                sentry__metrics_set_serialize(&statsd, metricValue);
                break;
            }

            sentry__stringbuilder_append(&statsd, "|");

            sentry__stringbuilder_append(
                &statsd, sentry__metrics_type_to_statsd(metricType));

            sentry__metrics_encode_statsd_tags(
                &statsd, sentry_value_get_by_key(metric, "tags"));

            sentry__stringbuilder_append(&statsd, "|T");

            uint64_t timestamp = sentry__iso8601_to_msec(sentry_value_as_string(
                sentry_value_get_by_key(metric, "timestamp")));
            sentry__metrics_timestamp_serialize(&statsd, timestamp / 1000);

            sentry__stringbuilder_append(&statsd, "\n");
        }
    }

    return sentry__stringbuilder_into_string(&statsd);
}

void
sentry__metrics_increment_add(sentry_value_t metric, sentry_value_t value)
{
    sentry_value_t metricValue = sentry_value_get_by_key(metric, "value");

    double current = sentry_value_as_double(metricValue);
    double val = sentry_value_as_double(value);

    sentry_value_set_by_key(
        metric, "value", sentry_value_new_double(current + val));
}

void
sentry__metrics_distribution_add(sentry_value_t metric, sentry_value_t value)
{
    sentry_value_t metricValue = sentry_value_get_by_key(metric, "value");

    sentry_value_append(metricValue, value);
}

void
sentry__metrics_gauge_add(sentry_value_t metric, sentry_value_t value)
{
    sentry_value_t metricValue = sentry_value_get_by_key(metric, "value");

    double min
        = sentry_value_as_double(sentry_value_get_by_key(metricValue, "min"));
    double max
        = sentry_value_as_double(sentry_value_get_by_key(metricValue, "max"));
    double sum
        = sentry_value_as_double(sentry_value_get_by_key(metricValue, "sum"));
    int32_t count
        = sentry_value_as_int32(sentry_value_get_by_key(metricValue, "count"));

    double val = sentry_value_as_double(value);

    sentry_value_set_by_key(metricValue, "last", value);
    sentry_value_set_by_key(
        metricValue, "min", sentry_value_new_double(MIN(min, val)));
    sentry_value_set_by_key(
        metricValue, "max", sentry_value_new_double(MAX(max, val)));
    sentry_value_set_by_key(
        metricValue, "sum", sentry_value_new_double(sum + val));
    sentry_value_set_by_key(
        metricValue, "count", sentry_value_new_int32(count + 1));
}

void
sentry__metrics_set_add(sentry_value_t metric, sentry_value_t value)
{
    sentry_value_t metricValue = sentry_value_get_by_key(metric, "value");

    size_t len = sentry_value_get_length(metricValue);
    for (size_t i = 0; i < len; i++) {
        sentry_value_t setItem = sentry_value_get_by_index(metricValue, i);
        if (sentry_value_as_int32(value) == sentry_value_as_int32(setItem)) {
            return;
        }
    }

    sentry_value_append(metricValue, value);
}

void
sentry__metrics_increment_serialize(
    sentry_stringbuilder_t *sb, sentry_value_t value)
{
    sentry__stringbuilder_append(sb, ":");
    sentry__stringbuilder_append(sb, sentry__value_stringify(value));
}

void
sentry__metrics_distribution_serialize(
    sentry_stringbuilder_t *sb, sentry_value_t value)
{
    size_t len = sentry_value_get_length(value);
    for (size_t i = 0; i < len; i++) {
        sentry__stringbuilder_append(sb, ":");
        sentry__stringbuilder_append(
            sb, sentry__value_stringify(sentry_value_get_by_index(value, i)));
    }
}

void
sentry__metrics_gauge_serialize(
    sentry_stringbuilder_t *sb, sentry_value_t value)
{
    sentry__stringbuilder_append(sb, ":");
    sentry__stringbuilder_append(
        sb, sentry__value_stringify(sentry_value_get_by_key(value, "last")));
    sentry__stringbuilder_append(sb, ":");
    sentry__stringbuilder_append(
        sb, sentry__value_stringify(sentry_value_get_by_key(value, "min")));
    sentry__stringbuilder_append(sb, ":");
    sentry__stringbuilder_append(
        sb, sentry__value_stringify(sentry_value_get_by_key(value, "max")));
    sentry__stringbuilder_append(sb, ":");
    sentry__stringbuilder_append(
        sb, sentry__value_stringify(sentry_value_get_by_key(value, "sum")));
    sentry__stringbuilder_append(sb, ":");
    sentry__stringbuilder_append(
        sb, sentry__value_stringify(sentry_value_get_by_key(value, "count")));
}

void
sentry__metrics_set_serialize(sentry_stringbuilder_t *sb, sentry_value_t value)
{
    size_t len = sentry_value_get_length(value);
    for (size_t i = 0; i < len; i++) {
        sentry__stringbuilder_append(sb, ":");
        sentry__stringbuilder_append(
            sb, sentry__value_stringify(sentry_value_get_by_index(value, i)));
    }
}

void
sentry__metrics_encode_statsd_tags(
    sentry_stringbuilder_t *sb, sentry_value_t tags)
{
    if (!sentry_value_is_null(tags)) {
        sentry__stringbuilder_append(sb, "|#");
        bool isFirst = true;
        for (size_t i = 0; i < sentry_value_get_length(tags); i++) {
            if (isFirst) {
                isFirst = false;
            } else {
                sentry__stringbuilder_append(sb, ",");
            }
            sentry_value_t tagItem = sentry_value_get_by_index(tags, i);

            char *tagKey = sentry__metrics_sanitize_tag_key(
                sentry_value_as_string(
                    sentry_value_get_by_key(tagItem, "key")));
            sentry__stringbuilder_append(sb, tagKey);
            sentry_free(tagKey);

            sentry__stringbuilder_append(sb, ":");

            char *tagValue = sentry__metrics_sanitize_tag_value(
                sentry_value_as_string(
                    sentry_value_get_by_key(tagItem, "value")));
            sentry__stringbuilder_append(sb, tagValue);
            sentry_free(tagValue);
        }
    }
}

void
sentry__metrics_timestamp_serialize(
    sentry_stringbuilder_t *sb, uint64_t timestamp)
{
    char timestampBuf[24];
    snprintf(timestampBuf, sizeof(timestampBuf), "%" PRIu64, timestamp);

    sentry__stringbuilder_append(sb, timestampBuf);
}

static void
set_tag_n(sentry_value_t item, sentry_slice_t tag, sentry_slice_t value)
{
    sentry_value_t tags = sentry_value_get_by_key(item, "tags");
    if (sentry_value_is_null(tags)) {
        tags = sentry_value_new_list();
        sentry_value_set_by_key(item, "tags", tags);
    }

    char *sKey = sentry__string_clone_max_n(tag.ptr, tag.len, 200);
    sentry_value_t tag_key
        = sKey ? sentry__value_new_string_owned(sKey) : sentry_value_new_null();

    char *sVal = sentry__string_clone_max_n(value.ptr, value.len, 200);
    sentry_value_t tag_value
        = sVal ? sentry__value_new_string_owned(sVal) : sentry_value_new_null();

    sentry_value_t newTag = sentry_value_new_object();
    sentry_value_set_by_key(newTag, "key", tag_key);
    sentry_value_set_by_key(newTag, "value", tag_value);

    sentry_value_append(tags, newTag);
}

static void
set_tag(sentry_value_t item, const char *tag, const char *value)
{
    const size_t tag_len = tag ? strlen(tag) : 0;
    const size_t value_len = value ? strlen(value) : 0;
    set_tag_n(item, (sentry_slice_t) { tag, tag_len },
        (sentry_slice_t) { value, value_len });
}

void
sentry_metric_set_tag_n(sentry_metric_t *metric, const char *tag,
    size_t tag_len, const char *value, size_t value_len)
{
    if (metric) {
        set_tag_n(metric->inner, (sentry_slice_t) { tag, tag_len },
            (sentry_slice_t) { value, value_len });
    }
}

void
sentry_metric_set_tag(
    sentry_metric_t *metric, const char *tag, const char *value)
{
    if (metric) {
        set_tag(metric->inner, tag, value);
    }
}

void
sentry_metric_set_unit_n(
    sentry_metric_t *metric, const char *unit, size_t unit_len)
{
    if (!metric) {
        return;
    }

    sentry_value_set_by_key(
        metric->inner, "unit", sentry_value_new_string_n(unit, unit_len));
}

void
sentry_metric_set_unit(sentry_metric_t *metric, const char *unit)
{
    const size_t unit_len = unit ? strlen(unit) : 0;

    sentry_metric_set_unit_n(metric, unit, unit_len);
}
