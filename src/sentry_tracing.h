#ifndef SENTRY_TRACING_H_INCLUDED
#define SENTRY_TRACING_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_value.h"

/**
 * Returns an object containing tracing information extracted from a
 * transaction (/span) which should be included in an event.
 * See https://develop.sentry.dev/sdk/event-payloads/transaction/#examples
 */
sentry_value_t sentry__span_get_trace_context(sentry_value_t span);

// /**
//  * This represents a span, which measures the duration of an event.
//  */
// typedef struct sentry_span_context_s {
//     sentry_uuid_t trace_id;
//     sentry_uuid_t span_id;
//     sentry_uuid_t parent_span_id; // nullable
//     // TODO: use status enum
//     int status;
//     char *op;
//     char *description;
//     bool sampled;
//     // TODO: add tags and additional data
//     uint64_t start_timestamp;
//     uint64_t end_timestamp;
// } sentry_span_context_t;

// /**
//  * A transaction context. Is a span context + a name, and is used to
//  * pre-populate new child spans and transactions.
//  */
// typedef struct sentry_transaction_context_s {
//     sentry_span_context_t span_context;
//     char *name;
// } sentry_transaction_context_t;

#endif
