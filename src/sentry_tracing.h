#ifndef SENTRY_TRACING_H_INCLUDED
#define SENTRY_TRACING_H_INCLUDED

// #include "sentry_boot.h"
// #include "sentry_path.h"
#include "sentry_utils.h"
#include "sentry_value.h"

/**
 *  The status of a Span or Transaction.
 */
typedef enum sentry_span_status_s {
    // Catch-all indeterminate status.
    SENTRY_SPAN_STATUS_UNDEFINED = 0,
    // The operation completed successfully.
    // HTTP status 100..299 + successful redirects from the 3xx range.
    SENTRY_SPAN_STATUS_OK = 0,
    // The operation was cancelled (typically by the user).
    SENTRY_SPAN_STATUS_CANCELLED,
    // Unknown. Any non-standard HTTP status code.
    // "We do not know whether the transaction failed or succeeded"
    SENTRY_SPAN_STATUS_UNKNOWN,
    // Client specified an invalid argument. 4xx.
    // Note that this differs from FailedPrecondition. InvalidArgument
    // indicates arguments that are problematic regardless of the
    // state of the system.
    SENTRY_SPAN_STATUS_INVALID_ARGUMENT,
    // Deadline expired before operation could complete.
    // For operations that change the state of the system, this error may be
    // returned even if the operation has been completed successfully.
    // HTTP redirect loops and 504 Gateway Timeout
    SENTRY_SPAN_STATUS_DEADLINE_EXCEEDED,
    // 404 Not Found. Some requested entity (file or directory) was not found.
    SENTRY_SPAN_STATUS_NOT_FOUND,
    // Already exists (409)
    // Some entity that we attempted to create already exists.
    SENTRY_SPAN_STATUS_ALREADY_EXISTS,
    // 403 Forbidden
    // The caller does not have permission to execute the specified operation.
    SENTRY_SPAN_STATUS_PERMISSION_DENIED,
    // 429 Too Many Requests
    // Some resource has been exhausted, perhaps a per-user quota or perhaps
    // the entire file system is out of space.
    SENTRY_SPAN_STATUS_RESOURCE_EXHAUSTED,
    // Operation was rejected because the system is not in a state required for
    // the operation's execution
    SENTRY_SPAN_STATUS_FAILED_PRECONDITION,
    // The operation was aborted, typically due to a concurrency issue.
    SENTRY_SPAN_STATUS_ABORTED,
    // Operation was attempted past the valid range.
    SENTRY_SPAN_STATUS_OUT_OF_RANGE,
    // 501 Not Implemented
    // Operation is not implemented or not enabled.
    SENTRY_SPAN_STATUS_UNIMPLEMENTED,
    // Other/generic 5xx.
    SENTRY_SPAN_STATUS_INTERNAL_ERROR,
    // 503 Service Unavailable
    SENTRY_SPAN_STATUS_UNAVAILABLE,
    // Unrecoverable data loss or corruption
    SENTRY_SPAN_STATUS_DATA_LOSS,
    // 401 Unauthorized (actually does mean unauthenticated according to RFC
    // 7235)
    // Prefer PermissionDenied if a user is logged in.
    SENTRY_SPAN_STATUS_UNAUTHENTICATED,
} sentry_span_status_t;

/**
 * This represents a span, which measures the duration of an event.
 */
typedef struct sentry_span_context_s {
    sentry_uuid_t trace_id;
    sentry_uuid_t span_id;
    sentry_uuid_t parent_span_id; // nullable
    sentry_span_status_t status;
    char *op;
    char *description;
    bool sampled;
    // A map or list of tags for this event. Each tag must be less than 200
    // characters.
    sentry_value_t tags;
    sentry_value_t data;
    uint64_t start_timestamp;
    uint64_t end_timestamp;
} sentry_span_context_t;

/**
 * A span.
 */
typedef struct sentry_span_s {
    sentry_span_context_t *context;

    bool parent_sampled;
    sentry_uuid_t event_id;
} sentry_span_t;

/**
 * A transaction context. Is a span context + a name, and is used to
 * pre-populate new child spans and transactions.
 */
typedef struct sentry_transaction_context_s {
    // TODO: just duplicate all of the properties?
    sentry_span_context_t *span_context;
    char *name;
} sentry_transaction_context_t;

/**
 * A transaction.
 */
typedef struct sentry_transaction_s {
    sentry_transaction_context_t *context;

    bool parent_sampled;
    sentry_uuid_t event_id;
    // Flat list of all child spans, nesting should be reconstructed via
    // `parent_span_id`s
    sentry_value_t spans;
} sentry_transaction_t;

sentry_transaction_t *sentry__transaction_incref(sentry_transaction_t *tx);

void sentry__transaction_decref(sentry_transaction_t *tx);

sentry_span_t *sentry__span_incref(sentry_span_t *span);

void sentry__span_decref(sentry_span_t *span);

/**
 * Returns an object containing tracing information extracted from a
 * transaction (/span) which should be included in an event.
 * See https://develop.sentry.dev/sdk/event-payloads/transaction/#examples
 */
sentry_value_t sentry__span_get_trace_context(sentry_span_t *span);

sentry_span_context_t *sentry__span_get_span_context(sentry_span_t *span);
#endif
