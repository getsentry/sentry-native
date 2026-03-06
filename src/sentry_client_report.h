#ifndef SENTRY_CLIENT_REPORT_H_INCLUDED
#define SENTRY_CLIENT_REPORT_H_INCLUDED

#include "sentry_boot.h"

/**
 * Discard reasons as specified in the Sentry SDK telemetry documentation.
 * https://develop.sentry.dev/sdk/telemetry/client-reports/
 */
typedef enum {
    SENTRY_DISCARD_REASON_QUEUE_OVERFLOW,
    SENTRY_DISCARD_REASON_RATELIMIT_BACKOFF,
    SENTRY_DISCARD_REASON_NETWORK_ERROR,
    SENTRY_DISCARD_REASON_SAMPLE_RATE,
    SENTRY_DISCARD_REASON_BEFORE_SEND,
    SENTRY_DISCARD_REASON_EVENT_PROCESSOR,
    SENTRY_DISCARD_REASON_SEND_ERROR,
    SENTRY_DISCARD_REASON_MAX
} sentry_discard_reason_t;

/**
 * Data categories for tracking discarded events.
 * These match the rate limiting categories defined at:
 * https://develop.sentry.dev/sdk/expected-features/rate-limiting/#definitions
 */
typedef enum {
    SENTRY_DATA_CATEGORY_ERROR,
    SENTRY_DATA_CATEGORY_SESSION,
    SENTRY_DATA_CATEGORY_TRANSACTION,
    SENTRY_DATA_CATEGORY_SPAN,
    SENTRY_DATA_CATEGORY_ATTACHMENT,
    SENTRY_DATA_CATEGORY_LOG_ITEM,
    SENTRY_DATA_CATEGORY_FEEDBACK,
    SENTRY_DATA_CATEGORY_MAX
} sentry_data_category_t;

/**
 * Record a discarded event with the given reason and category.
 * This function is thread-safe using atomic operations.
 */
void sentry__client_report_discard(sentry_discard_reason_t reason,
    sentry_data_category_t category, long quantity);

/**
 * Check if there are any pending discards to report.
 * Returns true if there are discards, false otherwise.
 */
bool sentry__client_report_has_pending(void);

/**
 * Create a client report envelope item and add it to the given envelope.
 * This atomically flushes all pending discard counters.
 * Returns the envelope item if added successfully, NULL otherwise.
 */
struct sentry_envelope_item_s *sentry__client_report_into_envelope(
    sentry_envelope_t *envelope);

/**
 * Record discards for all non-internal items in the envelope.
 * Skips client_report items. Each item is mapped to its data category.
 */
void sentry__client_report_discard_envelope(
    const sentry_envelope_t *envelope, sentry_discard_reason_t reason);

/**
 * Reset all client report counters to zero.
 * Called during SDK initialization to ensure a clean state.
 */
void sentry__client_report_reset(void);

#endif
