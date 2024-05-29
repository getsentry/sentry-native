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
#define MAX_TOTAL_WEIGHT 100000

#define FLUSHER_SLEEP_TIME_SEC 5

static bool g_aggregator_initialized = false;
static sentry_metrics_aggregator_t g_aggregator = { 0 };
static sentry_mutex_t g_aggregator_lock = SENTRY__MUTEX_INIT;

static int32_t g_total_buckets_weight = 0;

#ifndef SENTRY_INTEGRATIONTEST
static bool g_is_flush_scheduled = false;
#endif

static bool g_is_closed = false;

static sentry_bgworker_t *g_metrics_bgw = NULL;

static sentry_value_t
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

static const char *
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

static sentry_metric_type_t
sentry__metrics_type_from_string(sentry_value_t type)
{
    const char *type_str = sentry_value_as_string(type);

    if (sentry__string_eq(type_str, "counter")) {
        return SENTRY_METRIC_COUNTER;
    } else if (sentry__string_eq(type_str, "distribution")) {
        return SENTRY_METRIC_DISTRIBUTION;
    } else if (sentry__string_eq(type_str, "gauge")) {
        return SENTRY_METRIC_GAUGE;
    } else if (sentry__string_eq(type_str, "set")) {
        return SENTRY_METRIC_SET;
    } else {
        assert(!"invalid metric type");
        return -1;
    }
}

int
has_name_pattern_match(char c)
{
    return isalnum(c) || c == '_' || c == '\\' || c == '-' || c == '.';
}

int
has_unit_pattern_match(char c)
{
    return isalnum(c) || c == '_';
}

int
has_tag_key_pattern_match(char c)
{
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
    while (*ptr) {
        if (pattern_match_func(*ptr)) {
            sentry__stringbuilder_append_char(&sb, *ptr);
            ptr++;
        } else {
            sentry__stringbuilder_append(&sb, replacement);
            ptr++;

            // At this point, the last `ptr` value was either some replaced
            // ASCII or the start of a multi-byte sequence, which means `ptr`
            // points to the next character or the second byte of a multi-byte
            // sequence. If it is the latter, we must skip over all bytes in the
            // sequence so we only replace the whole character once.
            // Continuation bytes have the most significant bits set to `10`.
            while ((*ptr & 0xC0) == 0x80) {
                ptr++;
            }
        }
    }

    return sentry__stringbuilder_into_string(&sb);
}

char *
sentry__metrics_sanitize_name(const char *name)
{
    return sentry__metrics_sanitize(name, "_", has_name_pattern_match);
}

char *
sentry__metrics_sanitize_unit(const char *unit)
{
    return sentry__metrics_sanitize(unit, "", has_unit_pattern_match);
}

char *
sentry__metrics_sanitize_tag_key(const char *tag_key)
{
    return sentry__metrics_sanitize(tag_key, "", has_tag_key_pattern_match);
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

    sentry_value_t new_tag = sentry_value_new_object();
    sentry_value_set_by_key(new_tag, "key", tag_key);
    sentry_value_set_by_key(new_tag, "value", tag_value);

    sentry_value_append(tags, new_tag);
}

static void
set_tag(sentry_value_t item, const char *tag, const char *value)
{
    const size_t tag_len = tag ? strlen(tag) : 0;
    const size_t value_len = value ? strlen(value) : 0;
    set_tag_n(item, (sentry_slice_t) { tag, tag_len },
        (sentry_slice_t) { value, value_len });
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

    g_metrics_bgw = sentry__bgworker_new(NULL, NULL);
    sentry__bgworker_start(g_metrics_bgw);

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
        sentry__bgworker_shutdown(g_metrics_bgw, 1000);
        sentry__bgworker_decref(g_metrics_bgw);
        g_is_closed = true;
    }
    sentry__mutex_unlock(&g_aggregator_lock);
}

sentry_value_t
sentry__metrics_get_bucket_key(uint64_t timestamp)
{
    uint64_t seconds = timestamp / 1000000;
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

    uint64_t flushShiftUs = (uint64_t)((double)rnd / (double)UINT64_MAX
        * ROLLUP_IN_SECONDS * 1000000);

    uint64_t cutoff_timestamp
        = timestamp - ROLLUP_IN_SECONDS * 1000000 - flushShiftUs;

    uint64_t seconds = cutoff_timestamp / 1000000;

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
        sentry_value_t existing_bucket
            = sentry_value_get_by_index(aggregator->buckets, i);
        const char *key = sentry_value_as_string(
            sentry_value_get_by_key(existing_bucket, "key"));
        if (sentry__string_eq(key, sentry_value_as_string(bucketKey))) {
            return existing_bucket;
        }
    }

    // if there is no existing bucket with given key
    // create a new one and add it to aggregator
    sentry_value_t new_bucket = sentry_value_new_object();
    sentry_value_set_by_key(new_bucket, "key", bucketKey);
    sentry_value_set_by_key(new_bucket, "metrics", sentry_value_new_list());
    sentry_value_append(aggregator->buckets, new_bucket);
    return new_bucket;
}

char *
sentry__metrics_get_tags_key(sentry_value_t tags)
{
    sentry_stringbuilder_t sb;
    sentry__stringbuilder_init(&sb);

    for (size_t i = 0; i < sentry_value_get_length(tags); i++) {
        sentry_value_t tag_item = sentry_value_get_by_index(tags, i);
        if (sentry__stringbuilder_len(&sb) > 0) {
            sentry__stringbuilder_append(&sb, ",");
        }
        sentry__stringbuilder_append(&sb,
            sentry_value_as_string(sentry_value_get_by_key(tag_item, "key")));
        sentry__stringbuilder_append_char(&sb, '=');
        sentry__stringbuilder_append(&sb,
            sentry_value_as_string(sentry_value_get_by_key(tag_item, "value")));
    }

    return sentry__stringbuilder_into_string(&sb);
}

char *
sentry__metrics_get_metric_bucket_key(sentry_value_t metric)
{
    sentry_metric_type_t metric_type = sentry__metrics_type_from_string(
        sentry_value_get_by_key(metric, "type"));
    const char *type_prefix = sentry__metrics_type_to_statsd(metric_type);

    const char *metric_key
        = sentry_value_as_string(sentry_value_get_by_key(metric, "key"));

    const char *unit_name
        = sentry_value_as_string(sentry_value_get_by_key(metric, "unit"));

    char *serialized_tags
        = sentry__metrics_get_tags_key(sentry_value_get_by_key(metric, "tags"));

    size_t key_length = strlen(type_prefix) + strlen(metric_key)
        + strlen(unit_name) + strlen(serialized_tags) + 4;
    char *metric_bucket_key = sentry_malloc(key_length);
    size_t written = snprintf(metric_bucket_key, key_length, "%s_%s_%s_%s",
        type_prefix, metric_key, unit_name, serialized_tags);
    metric_bucket_key[written] = '\0';

    sentry_free(serialized_tags);

    return metric_bucket_key;
}

sentry_value_t
sentry__metrics_find_in_bucket(sentry_value_t bucket, const char *metricKey)
{
    sentry_value_t metrics = sentry_value_get_by_key(bucket, "metrics");
    size_t len = sentry_value_get_length(metrics);
    for (size_t i = 0; i < len; i++) {
        sentry_value_t existing_metric = sentry_value_get_by_index(metrics, i);
        const char *key = sentry_value_as_string(
            sentry_value_get_by_key(existing_metric, "key"));
        if (sentry__string_eq(key, metricKey)) {
            return sentry_value_get_by_key(existing_metric, "metric");
        }
    }

    return sentry_value_new_null();
}

typedef struct {
    int32_t delay;
} sentry_metric_flush_task_t;

static void
metric_flush_task(void *data, void *UNUSED(state))
{
    sentry_metric_flush_task_t *flush_task = (sentry_metric_flush_task_t *)data;
    sleep_s(flush_task->delay);

    SENTRY_WITH_METRICS_AGGREGATOR(aggregator)
    {
        sentry__metrics_aggregator_flush(aggregator, false);
    }

    if (!g_is_closed) {
        sentry__metrics_schedule_flush(FLUSHER_SLEEP_TIME_SEC);
    }
}

static void
metric_flush_cleanup_task(void *data)
{
    sentry_metric_flush_task_t *flush_task = (sentry_metric_flush_task_t *)data;
    sentry_free(flush_task);
}

void
sentry__metrics_schedule_flush(int32_t delay)
{
    sentry_metric_flush_task_t *flush_task
        = sentry_malloc(sizeof(sentry_metric_flush_task_t));
    flush_task->delay = delay;
    sentry__bgworker_submit(g_metrics_bgw, metric_flush_task,
        metric_flush_cleanup_task, flush_task);
}

void
sentry__metrics_aggregator_add(const sentry_metrics_aggregator_t *aggregator,
    sentry_value_t metric, double value)
{
    uint64_t timestamp = sentry__iso8601_to_usec(
        sentry_value_as_string(sentry_value_get_by_key(metric, "timestamp")));

    sentry_value_t bucket_key = sentry__metrics_get_bucket_key(timestamp);

    sentry_value_t bucket
        = sentry__metrics_get_or_add_bucket(aggregator, bucket_key);

    char *metric_bucket_key = sentry__metrics_get_metric_bucket_key(metric);

    sentry_value_t existing_metric
        = sentry__metrics_find_in_bucket(bucket, metric_bucket_key);

    int32_t added_weight;

    if (sentry_value_is_null(existing_metric)) {
        sentry_value_t new_bucket_item = sentry_value_new_object();
        sentry_value_set_by_key(
            new_bucket_item, "key", sentry_value_new_string(metric_bucket_key));
        sentry_value_set_by_key(new_bucket_item, "metric", metric);
        sentry_value_append(
            sentry_value_get_by_key(bucket, "metrics"), new_bucket_item);
        added_weight = sentry__metrics_get_weight(metric);
    } else {
        sentry_metric_type_t metric_type = sentry__metrics_type_from_string(
            sentry_value_get_by_key(metric, "type"));
        switch (metric_type) {
        case SENTRY_METRIC_COUNTER:
            sentry__metrics_increment_add(existing_metric, value);
            break;
        case SENTRY_METRIC_DISTRIBUTION:
            sentry__metrics_distribution_add(existing_metric, value);
            break;
        case SENTRY_METRIC_GAUGE:
            sentry__metrics_gauge_add(existing_metric, value);
            break;
        case SENTRY_METRIC_SET:
            sentry__metrics_set_add(existing_metric, (int32_t)value);
            break;
        default:
            SENTRY_WARN("uknown metric type");
        }
        added_weight = sentry__metrics_get_weight(existing_metric);
    }

    g_total_buckets_weight += added_weight;

    sentry_free(metric_bucket_key);

#ifdef SENTRY_INTEGRATIONTEST
    sentry__metrics_aggregator_flush(aggregator, true);
#elif SENTRY_UNITTEST
    // do nothing
#else
    bool is_owerweight = sentry__metrics_is_overweight(aggregator);
    if (is_owerweight || !g_is_flush_scheduled) {
        g_is_flush_scheduled = true;

        int32_t delay = is_owerweight ? 0 : FLUSHER_SLEEP_TIME_SEC;

        sentry__metrics_schedule_flush(delay);
    }
#endif
}

void
sentry__metrics_aggregator_flush(
    const sentry_metrics_aggregator_t *aggregator, bool force)
{
    if (!force && sentry__metrics_is_overweight(aggregator)) {
        force = true;
    }

    sentry_value_t flushable_buckets = sentry_value_new_list();

    uint64_t cutoff_timestamp
        = sentry__metrics_get_cutoff_timestamp(sentry__usec_time());

    size_t buckets_len = sentry_value_get_length(aggregator->buckets);

    // Since size_t is unsigned decrementing i after it is zero
    // yields the largest size_t value and the loop works correctly.
    for (size_t i = buckets_len - 1; i < buckets_len; i--) {
        sentry_value_t bucket
            = sentry_value_get_by_index_owned(aggregator->buckets, i);

        uint64_t bucket_timestamp = sentry__metrics_timestamp_from_string(
            sentry_value_as_string(sentry_value_get_by_key(bucket, "key")));

        if (force || bucket_timestamp < cutoff_timestamp) {
            sentry_value_append(flushable_buckets, bucket);
            g_total_buckets_weight -= sentry__metrics_get_bucket_weight(bucket);
            sentry_value_remove_by_index(aggregator->buckets, i);
        }
    }

    if (sentry_value_get_length(flushable_buckets) > 0) {
        char *encoded_metrics
            = sentry__metrics_encode_statsd(flushable_buckets);
        sentry__metrics_flush(encoded_metrics);
        sentry_free(encoded_metrics);
    }

    sentry_value_decref(flushable_buckets);
}

sentry_value_t
sentry__value_metric_new_n(sentry_slice_t name)
{
    sentry_value_t metric = sentry_value_new_object();

    sentry_value_set_by_key(
        metric, "key", sentry_value_new_string_n(name.ptr, name.len));

#if defined(SENTRY_UNITTEST) || defined(SENTRY_INTEGRATIONTEST)
    sentry_value_t now = sentry_value_new_string("2024-05-28T00:00:10Z");
#else
    sentry_value_t now = sentry__value_new_string_owned(
        sentry__usec_time_to_iso8601(sentry__usec_time()));
#endif

    sentry_value_set_by_key(metric, "timestamp", now);

    sentry_value_set_by_key(metric, "unit", sentry_value_new_string("none"));

    return metric;
}

void
sentry_metrics_emit_increment(
    const char *key, double value, const char *unit, ...)
{
    size_t key_len = key ? strlen(key) : 0;

    sentry_value_t metric
        = sentry__value_metric_new_n((sentry_slice_t) { key, key_len });
    if (sentry_value_is_null(metric)) {
        return;
    }

    sentry_value_set_by_key(
        metric, "type", sentry__metrics_type_to_string(SENTRY_METRIC_COUNTER));

    sentry_value_set_by_key(metric, "value", sentry_value_new_double(value));

    if (unit) {
        sentry_value_set_by_key(
            metric, "unit", sentry_value_new_string_n(unit, strlen(unit)));
    }

    va_list args;
    va_start(args, unit);
    const char *tag;
    const char *tag_value;
    while (1) {
        tag = va_arg(args, const char *);
        if (tag == NULL) {
            break;
        }
        tag_value = va_arg(args, const char *);
        set_tag(metric, tag, tag_value);
    }
    va_end(args);

    SENTRY_WITH_METRICS_AGGREGATOR(aggregator)
    {
        sentry__metrics_aggregator_add(aggregator, metric, value);
    }
}

void
sentry_metrics_emit_distribution(
    const char *key, double value, const char *unit, ...)
{
    size_t key_len = key ? strlen(key) : 0;

    sentry_value_t metric
        = sentry__value_metric_new_n((sentry_slice_t) { key, key_len });
    if (sentry_value_is_null(metric)) {
        return;
    }

    sentry_value_set_by_key(metric, "type",
        sentry__metrics_type_to_string(SENTRY_METRIC_DISTRIBUTION));

    sentry_value_t distribution_values = sentry_value_new_list();
    sentry_value_append(distribution_values, sentry_value_new_double(value));

    sentry_value_set_by_key(metric, "value", distribution_values);

    if (unit) {
        sentry_value_set_by_key(
            metric, "unit", sentry_value_new_string_n(unit, strlen(unit)));
    }

    va_list args;
    va_start(args, unit);
    const char *tag;
    const char *tag_value;
    while (1) {
        tag = va_arg(args, const char *);
        if (tag == NULL) {
            break;
        }
        tag_value = va_arg(args, const char *);
        set_tag(metric, tag, tag_value);
    }
    va_end(args);

    SENTRY_WITH_METRICS_AGGREGATOR(aggregator)
    {
        sentry__metrics_aggregator_add(aggregator, metric, value);
    }
}

void
sentry_metrics_emit_gauge(const char *key, double value, const char *unit, ...)
{
    size_t key_len = key ? strlen(key) : 0;

    sentry_value_t metric
        = sentry__value_metric_new_n((sentry_slice_t) { key, key_len });
    if (sentry_value_is_null(metric)) {
        return;
    }

    sentry_value_set_by_key(
        metric, "type", sentry__metrics_type_to_string(SENTRY_METRIC_GAUGE));

    sentry_value_t gauge_value = sentry_value_new_object();
    sentry_value_set_by_key(
        gauge_value, "last", sentry_value_new_double(value));
    sentry_value_set_by_key(gauge_value, "min", sentry_value_new_double(value));
    sentry_value_set_by_key(gauge_value, "max", sentry_value_new_double(value));
    sentry_value_set_by_key(gauge_value, "sum", sentry_value_new_double(value));
    sentry_value_set_by_key(gauge_value, "count", sentry_value_new_int32(1));

    sentry_value_set_by_key(metric, "value", gauge_value);

    if (unit) {
        sentry_value_set_by_key(
            metric, "unit", sentry_value_new_string_n(unit, strlen(unit)));
    }

    va_list args;
    va_start(args, unit);
    const char *tag;
    const char *tag_value;
    while (1) {
        tag = va_arg(args, const char *);
        if (tag == NULL) {
            break;
        }
        tag_value = va_arg(args, const char *);
        set_tag(metric, tag, tag_value);
    }
    va_end(args);

    SENTRY_WITH_METRICS_AGGREGATOR(aggregator)
    {
        sentry__metrics_aggregator_add(aggregator, metric, value);
    }
}

void
sentry_metrics_emit_set(const char *key, int32_t value, const char *unit, ...)
{
    size_t key_len = key ? strlen(key) : 0;

    sentry_value_t metric
        = sentry__value_metric_new_n((sentry_slice_t) { key, key_len });
    if (sentry_value_is_null(metric)) {
        return;
    }

    sentry_value_set_by_key(
        metric, "type", sentry__metrics_type_to_string(SENTRY_METRIC_SET));

    sentry_value_t set_values = sentry_value_new_list();
    sentry_value_append(set_values, sentry_value_new_int32(value));

    sentry_value_set_by_key(metric, "value", set_values);

    if (unit) {
        sentry_value_set_by_key(
            metric, "unit", sentry_value_new_string_n(unit, strlen(unit)));
    }

    va_list args;
    va_start(args, unit);
    const char *tag;
    const char *tag_value;
    while (1) {
        tag = va_arg(args, const char *);
        if (tag == NULL) {
            break;
        }
        tag_value = va_arg(args, const char *);
        set_tag(metric, tag, tag_value);
    }
    va_end(args);

    SENTRY_WITH_METRICS_AGGREGATOR(aggregator)
    {
        sentry__metrics_aggregator_add(aggregator, metric, value);
    }
}

char *
sentry__metrics_encode_statsd(sentry_value_t buckets)
{
    sentry_stringbuilder_t statsd;
    sentry__stringbuilder_init(&statsd);

    size_t buckets_len = sentry_value_get_length(buckets);
    for (size_t i = 0; i < buckets_len; i++) {
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

            char *unit = sentry__metrics_sanitize_unit(sentry_value_as_string(
                sentry_value_get_by_key(metric, "unit")));
            sentry__stringbuilder_append(&statsd, unit);
            sentry_free(unit);

            sentry_value_t metric_value
                = sentry_value_get_by_key(metric, "value");

            sentry_metric_type_t metric_type = sentry__metrics_type_from_string(
                sentry_value_get_by_key(metric, "type"));

            switch (metric_type) {
            case SENTRY_METRIC_COUNTER:
                sentry__metrics_increment_serialize(&statsd, metric_value);
                break;
            case SENTRY_METRIC_DISTRIBUTION:
                sentry__metrics_distribution_serialize(&statsd, metric_value);
                break;
            case SENTRY_METRIC_GAUGE:
                sentry__metrics_gauge_serialize(&statsd, metric_value);
                break;
            case SENTRY_METRIC_SET:
                sentry__metrics_set_serialize(&statsd, metric_value);
                break;
            }

            sentry__stringbuilder_append(&statsd, "|");

            sentry__stringbuilder_append(
                &statsd, sentry__metrics_type_to_statsd(metric_type));

            sentry__metrics_encode_statsd_tags(
                &statsd, sentry_value_get_by_key(metric, "tags"));

            sentry__stringbuilder_append(&statsd, "|T");

            uint64_t timestamp = sentry__iso8601_to_usec(sentry_value_as_string(
                sentry_value_get_by_key(metric, "timestamp")));
            sentry__metrics_timestamp_serialize(&statsd, timestamp / 1000000);

            sentry__stringbuilder_append(&statsd, "\n");
        }
    }

    return sentry__stringbuilder_into_string(&statsd);
}

void
sentry__metrics_increment_add(sentry_value_t metric, double value)
{
    sentry_value_t metric_value = sentry_value_get_by_key(metric, "value");

    double current = sentry_value_as_double(metric_value);

    sentry_value_set_by_key(
        metric, "value", sentry_value_new_double(current + value));
}

void
sentry__metrics_distribution_add(sentry_value_t metric, double value)
{
    sentry_value_t metric_value = sentry_value_get_by_key(metric, "value");

    sentry_value_append(metric_value, sentry_value_new_double(value));
}

void
sentry__metrics_gauge_add(sentry_value_t metric, double value)
{
    sentry_value_t metric_value = sentry_value_get_by_key(metric, "value");

    double min
        = sentry_value_as_double(sentry_value_get_by_key(metric_value, "min"));
    double max
        = sentry_value_as_double(sentry_value_get_by_key(metric_value, "max"));
    double sum
        = sentry_value_as_double(sentry_value_get_by_key(metric_value, "sum"));
    int32_t count
        = sentry_value_as_int32(sentry_value_get_by_key(metric_value, "count"));

    sentry_value_set_by_key(
        metric_value, "last", sentry_value_new_double(value));
    sentry_value_set_by_key(
        metric_value, "min", sentry_value_new_double(MIN(min, value)));
    sentry_value_set_by_key(
        metric_value, "max", sentry_value_new_double(MAX(max, value)));
    sentry_value_set_by_key(
        metric_value, "sum", sentry_value_new_double(sum + value));
    sentry_value_set_by_key(
        metric_value, "count", sentry_value_new_int32(count + 1));
}

void
sentry__metrics_set_add(sentry_value_t metric, int32_t value)
{
    sentry_value_t metric_value = sentry_value_get_by_key(metric, "value");

    size_t len = sentry_value_get_length(metric_value);
    for (size_t i = 0; i < len; i++) {
        sentry_value_t setItem = sentry_value_get_by_index(metric_value, i);
        if (value == sentry_value_as_int32(setItem)) {
            return;
        }
    }

    sentry_value_append(metric_value, sentry_value_new_int32(value));
}

int32_t
sentry__metrics_get_weight(sentry_value_t metric)
{
    int32_t weight;

    sentry_metric_type_t metric_type = sentry__metrics_type_from_string(
        sentry_value_get_by_key(metric, "type"));

    switch (metric_type) {
    case SENTRY_METRIC_COUNTER:
        weight = 1;
        break;
    case SENTRY_METRIC_DISTRIBUTION:
        weight = (int32_t)sentry_value_get_length(
            sentry_value_get_by_key(metric, "value"));
        break;
    case SENTRY_METRIC_GAUGE:
        weight = 5;
        break;
    case SENTRY_METRIC_SET:
        weight = (int32_t)sentry_value_get_length(
            sentry_value_get_by_key(metric, "value"));
        break;
    default:
        weight = 0;
    }

    return weight;
}

int32_t
sentry__metrics_get_bucket_weight(sentry_value_t bucket)
{
    int32_t weight = 0;

    sentry_value_t metrics = sentry_value_get_by_key(bucket, "metrics");
    for (size_t i = 0; i < sentry_value_get_length(metrics); i++) {
        sentry_value_t metric = sentry_value_get_by_key(
            sentry_value_get_by_index(metrics, i), "metric");
        weight += sentry__metrics_get_weight(metric);
    }

    return weight;
}

bool
sentry__metrics_is_overweight(const sentry_metrics_aggregator_t *aggregator)
{
    int32_t total_weight = (int32_t)sentry_value_get_length(aggregator->buckets)
        + g_total_buckets_weight;
    return total_weight >= MAX_TOTAL_WEIGHT;
}

void
sentry__metrics_increment_serialize(
    sentry_stringbuilder_t *sb, sentry_value_t value)
{
    sentry__stringbuilder_append(sb, ":");
    char *value_str = sentry__value_stringify(value);
    sentry__stringbuilder_append(sb, value_str);
    sentry_free(value_str);
}

void
sentry__metrics_distribution_serialize(
    sentry_stringbuilder_t *sb, sentry_value_t value)
{
    size_t len = sentry_value_get_length(value);
    for (size_t i = 0; i < len; i++) {
        sentry__stringbuilder_append(sb, ":");
        char *value_str
            = sentry__value_stringify(sentry_value_get_by_index(value, i));
        sentry__stringbuilder_append(sb, value_str);
        sentry_free(value_str);
    }
}

void
sentry__metrics_gauge_serialize(
    sentry_stringbuilder_t *sb, sentry_value_t value)
{
    sentry__stringbuilder_append(sb, ":");
    char *last_str
        = sentry__value_stringify(sentry_value_get_by_key(value, "last"));
    sentry__stringbuilder_append(sb, last_str);
    sentry_free(last_str);
    sentry__stringbuilder_append(sb, ":");
    char *min_str
        = sentry__value_stringify(sentry_value_get_by_key(value, "min"));
    sentry__stringbuilder_append(sb, min_str);
    sentry_free(min_str);
    sentry__stringbuilder_append(sb, ":");
    char *max_str
        = sentry__value_stringify(sentry_value_get_by_key(value, "max"));
    sentry__stringbuilder_append(sb, max_str);
    sentry_free(max_str);
    sentry__stringbuilder_append(sb, ":");
    char *sum_str
        = sentry__value_stringify(sentry_value_get_by_key(value, "sum"));
    sentry__stringbuilder_append(sb, sum_str);
    sentry_free(sum_str);
    sentry__stringbuilder_append(sb, ":");
    char *count_str
        = sentry__value_stringify(sentry_value_get_by_key(value, "count"));
    sentry__stringbuilder_append(sb, count_str);
    sentry_free(count_str);
}

void
sentry__metrics_set_serialize(sentry_stringbuilder_t *sb, sentry_value_t value)
{
    size_t len = sentry_value_get_length(value);
    for (size_t i = 0; i < len; i++) {
        sentry__stringbuilder_append(sb, ":");
        char *value_str
            = sentry__value_stringify(sentry_value_get_by_index(value, i));
        sentry__stringbuilder_append(sb, value_str);
        sentry_free(value_str);
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

            char *tagKey
                = sentry__metrics_sanitize_tag_key(sentry_value_as_string(
                    sentry_value_get_by_key(tagItem, "key")));
            sentry__stringbuilder_append(sb, tagKey);
            sentry_free(tagKey);

            sentry__stringbuilder_append(sb, ":");

            char *tagValue
                = sentry__metrics_sanitize_tag_value(sentry_value_as_string(
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
    char timestamp_buf[24];
    snprintf(timestamp_buf, sizeof(timestamp_buf), "%" PRIu64, timestamp);

    sentry__stringbuilder_append(sb, timestamp_buf);
}
