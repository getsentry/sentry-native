#ifndef SENTRY_DATABASE_H_INCLUDED
#define SENTRY_DATABASE_H_INCLUDED

#include "sentry_boot.h"

#include "sentry_path.h"

typedef struct {
    sentry_uuid_t uuid;
    sentry_path_t *run_path;
} sentry_run_t;

sentry_run_t *sentry__run_new(const sentry_path_t *database_path);

void sentry__run_free(sentry_run_t *run);

bool sentry__run_write_envelope(
    const sentry_run_t *run, sentry_envelope_t *envelope);

void sentry__enqueue_unsent_envelopes(const sentry_options_t *options);

#endif
