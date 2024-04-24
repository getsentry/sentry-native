#include "sentry_metrics.h"
#include "sentry.h"
#include "sentry_alloc.h"
#include "sentry_slice.h"
#include "sentry_string.h"
#include "sentry_utils.h"

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

    return metric;
}

sentry_metric_t *
sentry_metrics_new_increment_n(const char *key, size_t key_len, double value)
{
    sentry_metric_t *metric = SENTRY_MAKE(sentry_metric_t);
    if (!metric) {
        return NULL;
    }

    metric->inner = sentry__value_metric_new_n(
        (sentry_slice_t) { key, key_len });
    sentry_value_set_by_key(
        metric->inner, "type", sentry_value_new_string("counter"));

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

    metric->inner = sentry__value_metric_new_n(
        (sentry_slice_t) { key, key_len });
    sentry_value_set_by_key(
        metric->inner, "type", sentry_value_new_string("distribution"));

    sentry_value_t distributionValues = sentry_value_new_list();
    sentry_value_append(distributionValues, sentry_value_new_double(value));

    sentry_value_set_by_key(
        metric->inner, "value", distributionValues);

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

    metric->inner = sentry__value_metric_new_n(
        (sentry_slice_t) { key, key_len });
    sentry_value_set_by_key(
        metric->inner, "type", sentry_value_new_string("gauge"));

    sentry_value_t gaugeValue = sentry_value_new_object();
    sentry_value_set_by_key(
        gaugeValue, "last", sentry_value_new_double(value));
    sentry_value_set_by_key(
        gaugeValue, "mix", sentry_value_new_double(value));
    sentry_value_set_by_key(
        gaugeValue, "max", sentry_value_new_double(value));
    sentry_value_set_by_key(
        gaugeValue, "sum", sentry_value_new_double(value));
    sentry_value_set_by_key(
        gaugeValue, "count", sentry_value_new_int32(1));

    sentry_value_set_by_key(
        metric->inner, "value", gaugeValue);

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

    metric->inner = sentry__value_metric_new_n(
        (sentry_slice_t) { key, key_len });
    sentry_value_set_by_key(
        metric->inner, "type", sentry_value_new_string("set"));

    sentry_value_t setValues = sentry_value_new_list();
    sentry_value_append(setValues, sentry_value_new_int32(value));

    sentry_value_set_by_key(
        metric->inner, "value", setValues);

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

static void
set_tag_n(sentry_value_t item, sentry_slice_t tag, sentry_slice_t value)
{
    sentry_value_t tags = sentry_value_get_by_key(item, "tags");
    if (sentry_value_is_null(tags)) {
        tags = sentry_value_new_object();
        sentry_value_set_by_key(item, "tags", tags);
    }
    char *s = sentry__string_clone_max_n(value.ptr, value.len, 200);
    sentry_value_t tag_value
        = s ? sentry__value_new_string_owned(s) : sentry_value_new_null();
    sentry_value_set_by_key_n(tags, tag.ptr, tag.len, tag_value);
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
sentry_metric_set_tag(sentry_metric_t *metric, const char *tag, const char *value)
{
    if (metric) {
        set_tag(metric->inner, tag, value);
    }
}

void
sentry_metric_set_tag_n(sentry_metric_t *metric, const char *tag, size_t tag_len,
    const char *value, size_t value_len)
{
    if (metric) {
        set_tag_n(metric->inner, (sentry_slice_t) { tag, tag_len },
            (sentry_slice_t) { value, value_len });
    }
}