#ifndef SENTRY_METRICS_H_INCLUDED
#define SENTRY_METRICS_H_INCLUDED

#include "sentry_value.h"

/**
 * A span.
 */
typedef struct sentry_metric_s {
    sentry_value_t inner;
} sentry_metric_t;

void sentry__metric_free(sentry_metric_t *metric);

#endif
