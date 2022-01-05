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
#endif
