#ifndef SENTRY_CLIENT_REPORT_H_INCLUDED
#define SENTRY_CLIENT_REPORT_H_INCLUDED

#include "sentry_boot.h"

/**
 * Discard reasons as specified in the Sentry SDK telemetry documentation.
 * https://develop.sentry.dev/sdk/telemetry/client-reports/
 */
typedef enum {
    SENTRY_DISCARD_REASON_QUEUE_OVERFLOW,
    SENTRY_DISCARD_REASON_CACHE_OVERFLOW,
    SENTRY_DISCARD_REASON_RATELIMIT_BACKOFF,
    SENTRY_DISCARD_REASON_NETWORK_ERROR,
    SENTRY_DISCARD_REASON_SAMPLE_RATE,
    SENTRY_DISCARD_REASON_BEFORE_SEND,
    SENTRY_DISCARD_REASON_SEND_ERROR,
    SENTRY_DISCARD_REASON_MAX
} sentry_discard_reason_t;

/**
 * Data categories for tracking discarded events.
 * These match the rate limiting categories defined at:
 * https://develop.sentry.dev/sdk/expected-features/rate-limiting/#definitions
 *
 * The `_BYTE` categories track the byte size of discarded telemetry alongside
 * its item count, see
 * https://develop.sentry.dev/sdk/telemetry/client-reports/#log-byte-outcomes
 */
typedef enum {
    SENTRY_DATA_CATEGORY_ERROR,
    SENTRY_DATA_CATEGORY_SESSION,
    SENTRY_DATA_CATEGORY_TRANSACTION,
    SENTRY_DATA_CATEGORY_ATTACHMENT,
    SENTRY_DATA_CATEGORY_LOG_ITEM,
    SENTRY_DATA_CATEGORY_FEEDBACK,
    SENTRY_DATA_CATEGORY_TRACE_METRIC,
    SENTRY_DATA_CATEGORY_REPLAY,
    SENTRY_DATA_CATEGORY_LOG_BYTE,
    SENTRY_DATA_CATEGORY_TRACE_METRIC_BYTE,
    SENTRY_DATA_CATEGORY_MAX
} sentry_data_category_t;

/**
 * Consumed discard counts, used to restore them on send failure.
 *
 * NOTE: `long` is 32-bit on Windows, so the `_BYTE` categories saturate at 2GiB
 * of discarded telemetry between two client reports. Since the counters are
 * flushed with every outgoing envelope, reaching that is not realistic.
 */
typedef struct {
    long counts[SENTRY_DISCARD_REASON_MAX][SENTRY_DATA_CATEGORY_MAX];
} sentry_client_report_t;

/**
 * Record a discarded event with the given reason and category.
 * This function is thread-safe using atomic operations.
 */
void sentry__client_report_discard(sentry_discard_reason_t reason,
    sentry_data_category_t category, long quantity);

/**
 * Record `quantity` discarded items of `category`, plus `bytes` under the
 * category's byte counterpart (`log_byte` for logs, `trace_metric_byte` for
 * metrics). `bytes` is ignored for categories that have no byte counterpart,
 * so callers that discard items of an unknown category can use this
 * unconditionally.
 */
void sentry__client_report_discard_with_bytes(sentry_discard_reason_t reason,
    sentry_data_category_t category, long quantity, long bytes);

/**
 * Check if there are any pending discards to report.
 */
bool sentry__client_report_has_pending(void);

/**
 * Atomically read and clear all pending discard counters into `report`.
 * Returns true if any counts were pending.
 */
bool sentry__client_report_save(sentry_client_report_t *report);

/**
 * Re-add a client report's counts back to the global counters.
 */
void sentry__client_report_restore(sentry_client_report_t *report);

/**
 * Reset all pending discard counters to zero.
 * Called during sentry_init() so that stale counts from a previous session
 * do not leak into the new one.
 */
void sentry__client_report_reset(void);

#endif
