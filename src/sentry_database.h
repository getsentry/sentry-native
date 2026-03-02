#ifndef SENTRY_DATABASE_H_INCLUDED
#define SENTRY_DATABASE_H_INCLUDED

#include "sentry_boot.h"

#include "sentry_path.h"
#include "sentry_session.h"

typedef struct sentry_run_s {
    sentry_uuid_t uuid;
    sentry_path_t *run_path;
    sentry_path_t *session_path;
    sentry_path_t *external_path;
    sentry_path_t *cache_path;
    sentry_filelock_t *lock;
    long refcount;
} sentry_run_t;

/**
 * This creates a new application run including its associated directory and
 * lockfile:
 * * `<database>/<uuid>.run/`
 * * `<database>/<uuid>.run.lock`
 */
sentry_run_t *sentry__run_new(const sentry_path_t *database_path);

/**
 * Increment the refcount and return the run pointer.
 */
sentry_run_t *sentry__run_incref(sentry_run_t *run);

/**
 * This will clean up all the files belonging to this run.
 */
void sentry__run_clean(sentry_run_t *run);

/**
 * Free the previously allocated run.
 * Make sure to call `sentry__run_clean` first, to not leave any files or
 * directories laying around.
 */
void sentry__run_free(sentry_run_t *run);

/**
 * This will serialize and write the given envelope to disk into a file named
 * like so:
 * `<database>/<uuid>.run/<event-uuid>.envelope`
 */
bool sentry__run_write_envelope(
    const sentry_run_t *run, sentry_envelope_t *envelope);

/**
 * This will serialize and write the given envelope to disk into a file named
 * like so:
 * `<database>/external/<event-uuid>.envelope`
 */
bool sentry__run_write_external(
    const sentry_run_t *run, const sentry_envelope_t *envelope);

/**
 * This will serialize and write the given session to disk into a file named:
 * `<database>/<uuid>.run/session.json`
 */
bool sentry__run_write_session(
    const sentry_run_t *run, const sentry_session_t *session);

/**
 * This will remove any previously created session file.
 * See `sentry__run_write_session`.
 */
bool sentry__run_clear_session(const sentry_run_t *run);

/**
 * This will serialize and write the given envelope to disk into the cache
 * directory. When retry_count >= 0 the filename uses retry format
 * `<ts>-<count>-<uuid>.envelope`, otherwise `<uuid>.envelope`.
 */
bool sentry__run_write_cache(const sentry_run_t *run,
    const sentry_envelope_t *envelope, int retry_count);

/**
 * Moves a file into the cache directory. When retry_count >= 0 the
 * destination uses retry format `<ts>-<count>-<uuid>.envelope`,
 * otherwise the original filename is preserved.
 */
bool sentry__run_move_cache(
    const sentry_run_t *run, const sentry_path_t *src, int retry_count);

/**
 * Builds a cache path. When count >= 0 the result is
 * `<db>/cache/<ts>-<count>-<uuid>.envelope`, otherwise
 * `<db>/cache/<uuid>.envelope`.
 */
sentry_path_t *sentry__run_make_cache_path(
    const sentry_run_t *run, uint64_t ts, int count, const char *uuid);

/**
 * This function is essential to send crash reports from previous runs of the
 * program.
 * More specifically, this function will iterate over all the  directories
 * inside the `database_path`. Directories matching `<database>/<uuid>.run/`
 * will be locked, and any files named  `<event-uuid>.envelope` or
 * `session.json` will be queued for sending to the  backend. The files and
 * directories matching these criteria will be deleted afterwards.
 * The following heuristic is applied to all unclosed sessions: If the session
 * was started before the timestamp given by `last_crash`, the session is closed
 * as "crashed" with an appropriate duration.
 */
void sentry__process_old_runs(
    const sentry_options_t *options, uint64_t last_crash);

/**
 * Parses a retry cache filename: `<ts>-<count>-<uuid>.envelope`.
 * Returns false for plain cache filenames (`<uuid>.envelope`).
 */
bool sentry__parse_cache_filename(const char *filename, uint64_t *ts_out,
    int *count_out, const char **uuid_out);

/**
 * Cleans up the cache based on options.cache_max_items,
 * options.cache_max_size and options.cache_max_age.
 */
void sentry__cleanup_cache(const sentry_options_t *options);

/**
 * This will write the current ISO8601 formatted timestamp into the
 * `<database>/last_crash` file.
 */
bool sentry__write_crash_marker(const sentry_options_t *options);

/**
 * This will check whether the `<database>/last_crash` file exists.
 */
bool sentry__has_crash_marker(const sentry_options_t *options);

/**
 * This will remove the `<database>/last_crash` file.
 */
bool sentry__clear_crash_marker(const sentry_options_t *options);

/**
 * Removes a large attachment file only if it lives under
 * `<db_path>/attachments/`. This prevents deletion of user-owned
 * original files that were referenced directly (not copied by the
 * crash persistence path).
 */
void sentry__db_remove_large_attachment(
    const sentry_path_t *db_path, const sentry_path_t *path);

#endif
