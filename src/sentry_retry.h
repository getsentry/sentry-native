#ifndef SENTRY_RETRY_H_INCLUDED
#define SENTRY_RETRY_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_path.h"

typedef struct {
    sentry_path_t *database_path;
    int max_attempts;
    bool cache_keep;
} sentry_retry_t;

sentry_retry_t *sentry__retry_new(
    const sentry_path_t *database_path, int max_attempts, bool cache_keep);

void sentry__retry_free(sentry_retry_t *retry);

bool sentry__retry_write_envelope(
    const sentry_retry_t *retry, const sentry_envelope_t *envelope);

void sentry__retry_remove_envelope(
    const sentry_retry_t *retry, const sentry_uuid_t *envelope_id);

void sentry__retry_cache_envelope(
    const sentry_retry_t *retry, const sentry_uuid_t *envelope_id);

void sentry__retry_process_envelopes(const sentry_options_t *options);

#endif
