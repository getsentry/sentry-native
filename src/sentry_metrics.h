#ifndef SENTRY_METRICS_H_INCLUDED
#define SENTRY_METRICS_H_INCLUDED

#include "sentry_json.h"
#include "sentry_value.h"

/**
 * Sentry metric types.
 */
typedef enum sentry_metric_type_e {
    SENTRY_METRIC_COUNTER = 0,
    SENTRY_METRIC_DISTRIBUTION = 1,
    SENTRY_METRIC_GAUGE = 2,
    SENTRY_METRIC_SET = 3,
} sentry_metric_type_t;

/**
 * A metric.
 */
typedef struct sentry_metric_s {
    sentry_value_t inner;
    sentry_value_t value;
    sentry_metric_type_t type;
} sentry_metric_t;

/**
 * A metrics aggregator.
 */
typedef struct sentry_metrics_aggregator_s {
    sentry_value_t buckets;
} sentry_metrics_aggregator_t;

const char *sentry__metrics_sanitize_tag_value(const char *tag_value);

/**
 * Acquires a lock on the global metrics aggregator.
 */
sentry_metrics_aggregator_t *sentry__metrics_aggregator_lock(void);

/**
 * Releases the lock on the global metrics aggregator.
 */
void sentry__metrics_aggregator_unlock(void);

/**
 * Free all the data attached to the global metrics aggregator
 */
void sentry__metrics_aggregator_cleanup(void);

void sentry__metrics_aggregator_add(
    const sentry_metrics_aggregator_t *aggregator, sentry_metric_t *metric);

void sentry__metrics_aggregator_flush(
    const sentry_metrics_aggregator_t *aggregator, bool force);

void sentry__metric_free(sentry_metric_t *metric);

void sentry__metrics_increment_add(sentry_value_t metric, sentry_value_t value);
void sentry__metrics_distribution_add(
    sentry_value_t metric, sentry_value_t value);
void sentry__metrics_gauge_add(sentry_value_t metric, sentry_value_t value);
void sentry__metrics_set_add(sentry_value_t metric, sentry_value_t value);

void sentry__metrics_increment_serialize(
    sentry_stringbuilder_t *sb, sentry_value_t value);
void sentry__metrics_distribution_serialize(
    sentry_stringbuilder_t *sb, sentry_value_t value);
void sentry__metrics_gauge_serialize(
    sentry_stringbuilder_t *sb, sentry_value_t value);
void sentry__metrics_set_serialize(
    sentry_stringbuilder_t *sb, sentry_value_t value);

const char *sentry__metrics_encode_statsd(sentry_value_t buckets);

void sentry__metrics_tags_serialize(
    sentry_stringbuilder_t *sb, sentry_value_t tags);

void sentry__metrics_timestamp_serialize(
    sentry_stringbuilder_t *sb, uint64_t timestamp);

/**
 * Convenience macros to automatically lock/unlock a metrics aggregator
 * inside a code block.
 */
#define SENTRY_WITH_METRICS_AGGREGATOR(Aggregator)                             \
    for (const sentry_metrics_aggregator_t *Aggregator                         \
         = sentry__metrics_aggregator_lock();                                  \
         Aggregator; sentry__metrics_aggregator_unlock(), Aggregator = NULL)

#endif
