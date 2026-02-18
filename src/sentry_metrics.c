#include "sentry_metrics.h"
#include "sentry_batcher.h"
#include "sentry_core.h"
#include "sentry_envelope.h"
#include "sentry_options.h"
#include "sentry_scope.h"
#include "sentry_utils.h"
#include "sentry_value.h"

typedef enum {
    SENTRY_METRIC_COUNT,
    SENTRY_METRIC_GAUGE,
    SENTRY_METRIC_DISTRIBUTION,
} sentry_metric_type_t;

static sentry_batcher_t g_batcher = {
    {
        {
            .index = 0,
            .adding = 0,
            .sealed = 0,
        },
        {
            .index = 0,
            .adding = 0,
            .sealed = 0,
        },
    },
    .active_idx = 0,
    .flushing = 0,
    .thread_state = SENTRY_BATCHER_THREAD_STOPPED,
    .batch_func = sentry__envelope_add_metrics,
};

static const char *
metric_type_string(sentry_metric_type_t type)
{
    switch (type) {
    case SENTRY_METRIC_COUNT:
        return "counter";
    case SENTRY_METRIC_GAUGE:
        return "gauge";
    case SENTRY_METRIC_DISTRIBUTION:
        return "distribution";
    default:
        return "unknown";
    }
}

static sentry_value_t
construct_metric(sentry_metric_type_t type, const char *name,
    sentry_value_t value, const char *unit, sentry_value_t user_attributes)
{
    sentry_value_t metric = sentry_value_new_object();

    uint64_t usec_time = sentry__usec_time();
    sentry_value_set_by_key(metric, "timestamp",
        sentry_value_new_double((double)usec_time / 1000000.0));
    sentry_value_set_by_key(
        metric, "type", sentry_value_new_string(metric_type_string(type)));
    sentry_value_set_by_key(metric, "name", sentry_value_new_string(name));
    sentry_value_set_by_key(metric, "value", value);
    if (unit && unit[0] != '\0') {
        sentry_value_set_by_key(metric, "unit", sentry_value_new_string(unit));
    }

    sentry_value_t attributes
        = sentry_value_get_type(user_attributes) == SENTRY_VALUE_TYPE_OBJECT
        ? sentry__value_clone(user_attributes)
        : sentry_value_new_object();
    sentry_value_decref(user_attributes);

    sentry__apply_attributes(metric, attributes);

    if (sentry_value_get_length(attributes) > 0) {
        sentry_value_set_by_key(metric, "attributes", attributes);
    } else {
        sentry_value_decref(attributes);
    }

    return metric;
}

static sentry_metrics_result_t
record_metric(sentry_metric_type_t type, const char *name, sentry_value_t value,
    const char *unit, sentry_value_t attributes)
{
    bool enable_metrics = false;
    SENTRY_WITH_OPTIONS (options) {
        if (options->enable_metrics)
            enable_metrics = true;
    }
    if (enable_metrics) {
        bool discarded = false;
        sentry_value_t metric
            = construct_metric(type, name, value, unit, attributes);
        SENTRY_WITH_OPTIONS (options) {
            if (options->before_send_metric_func) {
                metric = options->before_send_metric_func(
                    metric, options->before_send_metric_data);
                if (sentry_value_is_null(metric)) {
                    SENTRY_DEBUG("metric was discarded by the "
                                 "`before_send_metric` hook");
                    discarded = true;
                }
            }
        }
        if (discarded) {
            return SENTRY_METRICS_RESULT_DISCARD;
        }
        if (!sentry__batcher_enqueue(&g_batcher, metric)) {
            sentry_value_decref(metric);
            return SENTRY_METRICS_RESULT_FAILED;
        }
        return SENTRY_METRICS_RESULT_SUCCESS;
    }
    sentry_value_decref(value);
    sentry_value_decref(attributes);
    return SENTRY_METRICS_RESULT_DISABLED;
}

sentry_metrics_result_t
sentry_metrics_count(const char *name, int64_t value, sentry_value_t attributes)
{
    return record_metric(SENTRY_METRIC_COUNT, name,
        sentry_value_new_int64(value), NULL, attributes);
}

sentry_metrics_result_t
sentry_metrics_gauge(
    const char *name, double value, const char *unit, sentry_value_t attributes)
{
    return record_metric(SENTRY_METRIC_GAUGE, name,
        sentry_value_new_double(value), unit, attributes);
}

sentry_metrics_result_t
sentry_metrics_distribution(
    const char *name, double value, const char *unit, sentry_value_t attributes)
{
    return record_metric(SENTRY_METRIC_DISTRIBUTION, name,
        sentry_value_new_double(value), unit, attributes);
}

void
sentry__metrics_startup(const sentry_options_t *options)
{
    sentry__batcher_startup(&g_batcher, options);
}

bool
sentry__metrics_shutdown_begin(void)
{
    SENTRY_DEBUG("beginning metrics system shutdown");
    return sentry__batcher_shutdown_begin(&g_batcher);
}

void
sentry__metrics_shutdown_wait(uint64_t timeout)
{
    sentry__batcher_shutdown_wait(&g_batcher, timeout);
    SENTRY_DEBUG("metrics system shutdown complete");
}

void
sentry__metrics_flush_crash_safe(void)
{
    SENTRY_DEBUG("crash-safe metrics flush");
    sentry__batcher_flush_crash_safe(&g_batcher);
    SENTRY_DEBUG("crash-safe metrics flush complete");
}

void
sentry__metrics_force_flush_begin(void)
{
    sentry__batcher_force_flush_begin(&g_batcher);
}

void
sentry__metrics_force_flush_wait(void)
{
    sentry__batcher_force_flush_wait(&g_batcher);
}

#ifdef SENTRY_UNITTEST
/**
 * Wait for the metrics batching thread to be ready.
 * This is a test-only helper to avoid race conditions in tests.
 */
void
sentry__metrics_wait_for_thread_startup(void)
{
    sentry__batcher_wait_for_thread_startup(&g_batcher);
}
#endif
