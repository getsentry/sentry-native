#ifndef SENTRY_TRACING_H_INCLUDED
#define SENTRY_TRACING_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_value.h"

/**
 * A span.
 */
typedef struct sentry_span_s {
    sentry_value_t inner;
} sentry_span_t;

/**
 * A transaction context.
 */
typedef struct sentry_transaction_context_s {
    sentry_value_t inner;
} sentry_transaction_context_t;

/**
 * A transaction.
 */
typedef struct sentry_transaction_s {
    sentry_value_t inner;
} sentry_transaction_t;

/**
 * Returns an object containing tracing information extracted from a
 * transaction (/span) which should be included in an event.
 * See https://develop.sentry.dev/sdk/event-payloads/transaction/#examples
 */
sentry_value_t sentry__span_get_trace_context(sentry_value_t span);

sentry_value_t sentry__span_get_span_context(sentry_value_t span);
#endif
