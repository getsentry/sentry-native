#ifndef SENTRY_RETRY_H_INCLUDED
#define SENTRY_RETRY_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_path.h"

/**
 * Write an envelope to the retry directory for later retry.
 * Creates `<database>/retry/<event-uuid>.<attempt>.envelope`.
 * If attempts limit is exceeded, moves to cache (if cache_keep) or discards.
 */
bool sentry__retry_write_envelope(const sentry_path_t *database_path,
    const sentry_envelope_t *envelope, int attempts, bool cache_keep);

/**
 * Remove an envelope from the retry directory.
 * Called after successful send or permanent failure.
 */
void sentry__retry_remove_envelope(
    const sentry_path_t *database_path, const sentry_uuid_t *envelope_id);

/**
 * Move an envelope from retry to cache directory.
 * Called after successful send when cache_keep is enabled.
 */
void sentry__retry_cache_envelope(
    const sentry_path_t *database_path, const sentry_uuid_t *envelope_id);

/**
 * Retry sending envelopes from the retry directory.
 * Called on startup to send previously failed envelopes.
 */
void sentry__retry_process_envelopes(const sentry_options_t *options);

#endif
