#ifndef SENTRY_DATABASE_H_INCLUDED
#define SENTRY_DATABASE_H_INCLUDED

#include "sentry_boot.h"

#include "sentry_path.h"
#include "sentry_session.h"

typedef struct {
    sentry_uuid_t uuid;
    sentry_path_t *run_path;
    sentry_path_t *session_path;
    sentry_filelock_t lock;
} sentry_run_t;

sentry_run_t *sentry__run_new(const sentry_path_t *database_path);

/**
 * This will clean up all the files belonging to this to this run.
 */
void sentry__run_clean(sentry_run_t *run);
void sentry__run_free(sentry_run_t *run);

bool sentry__run_write_envelope(
    const sentry_run_t *run, const sentry_envelope_t *envelope);
bool sentry__run_write_session(
    const sentry_run_t *run, const sentry_session_t *session);
bool sentry__run_clear_session(const sentry_run_t *run);

void sentry__process_old_runs(const sentry_options_t *options);

/**
 * This will write the current timestamp (ISO formatted) into the
 * `${database_path}/last_crash` file.
 */
bool sentry__write_crash_marker(const sentry_options_t *options);

#endif
