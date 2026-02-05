#include "sentry_retry.h"
#include "sentry_alloc.h"
#include "sentry_core.h"
#include "sentry_envelope.h"
#include "sentry_logger.h"
#include "sentry_options.h"
#include "sentry_string.h"
#include "sentry_transport.h"
#include "sentry_uuid.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static sentry_path_t *
get_retry_path(const sentry_path_t *database_path)
{
    return sentry__path_join_str(database_path, "retry");
}

static sentry_path_t *
get_cache_path(const sentry_path_t *database_path)
{
    return sentry__path_join_str(database_path, "cache");
}

/**
 * Build a retry envelope filename with attempt number.
 * Format: <timestamp>-<counter>-<uuid>.envelope
 */
static char *
make_retry_filename(const sentry_uuid_t *envelope_id, int attempt)
{
    char uuid_str[37];
    sentry_uuid_as_string(envelope_id, uuid_str);

    // <timestamp>-<counter>-<uuid>.envelope
    char *filename = sentry_malloc(80);
    if (filename) {
        snprintf(filename, 80, "%llu-%d-%s.envelope",
            (unsigned long long)time(NULL), attempt, uuid_str);
    }
    return filename;
}

/**
 * Find an existing retry file for the given envelope ID.
 * Returns the attempt number (1-5), or 0 if not found.
 */
static int
find_retry_attempt(
    const sentry_path_t *retry_path, const sentry_uuid_t *envelope_id)
{
    char uuid_str[37];
    sentry_uuid_as_string(envelope_id, uuid_str);

    int found_attempt = 0;
    sentry_pathiter_t *iter = sentry__path_iter_directory(retry_path);
    const sentry_path_t *file;
    while (iter && (file = sentry__pathiter_next(iter)) != NULL) {
        const char *filename = sentry__path_filename(file);
        if (!filename) {
            continue;
        }

        // Parse <timestamp>-<counter>-<uuid>.envelope
        const char *first_dash = strchr(filename, '-');
        if (!first_dash) {
            continue;
        }
        int attempt = atoi(first_dash + 1);
        const char *second_dash = strchr(first_dash + 1, '-');
        if (!second_dash) {
            continue;
        }
        const char *uuid_start = second_dash + 1;
        if (strncmp(uuid_start, uuid_str, 36) == 0
            && strcmp(uuid_start + 36, ".envelope") == 0) {
            if (attempt > found_attempt) {
                found_attempt = attempt;
            }
        }
    }
    sentry__pathiter_free(iter);
    return found_attempt;
}

/**
 * Remove retry file for the given envelope ID (any attempt number).
 */
static void
remove_retry_file(
    const sentry_path_t *retry_path, const sentry_uuid_t *envelope_id)
{
    char uuid_str[37];
    sentry_uuid_as_string(envelope_id, uuid_str);

    sentry_pathiter_t *iter = sentry__path_iter_directory(retry_path);
    const sentry_path_t *file;
    while (iter && (file = sentry__pathiter_next(iter)) != NULL) {
        const char *filename = sentry__path_filename(file);
        if (!filename) {
            continue;
        }

        // Match <timestamp>-<counter>-<uuid>.envelope
        const char *first_dash = strchr(filename, '-');
        if (!first_dash) {
            continue;
        }
        const char *second_dash = strchr(first_dash + 1, '-');
        if (!second_dash) {
            continue;
        }
        const char *uuid_start = second_dash + 1;
        if (strncmp(uuid_start, uuid_str, 36) == 0
            && strcmp(uuid_start + 36, ".envelope") == 0) {
            sentry__path_remove(file);
            break;
        }
    }
    sentry__pathiter_free(iter);
}

/**
 * Write envelope to cache directory.
 */
static bool
write_to_cache(
    const sentry_path_t *database_path, const sentry_envelope_t *envelope)
{
    sentry_path_t *cache_path = get_cache_path(database_path);
    if (!cache_path) {
        return false;
    }

    if (sentry__path_create_dir_all(cache_path) != 0) {
        sentry__path_free(cache_path);
        return false;
    }

    sentry_uuid_t event_id = sentry__envelope_get_event_id(envelope);
    if (sentry_uuid_is_nil(&event_id)) {
        event_id = sentry_uuid_new_v4();
    }

    char *filename = sentry__uuid_as_filename(&event_id, ".envelope");
    if (!filename) {
        sentry__path_free(cache_path);
        return false;
    }

    sentry_path_t *output_path = sentry__path_join_str(cache_path, filename);
    sentry_free(filename);
    sentry__path_free(cache_path);

    if (!output_path) {
        return false;
    }

    int rv = sentry_envelope_write_to_path(envelope, output_path);
    sentry__path_free(output_path);
    return rv == 0;
}

bool
sentry__retry_write_envelope(const sentry_path_t *database_path,
    const sentry_envelope_t *envelope, int attempts, bool cache_keep)
{
    if (!database_path || !envelope || attempts <= 0) {
        return false;
    }

    sentry_path_t *retry_path = get_retry_path(database_path);
    if (!retry_path) {
        return false;
    }

    if (sentry__path_create_dir_all(retry_path) != 0) {
        SENTRY_ERRORF("mkdir failed: \"%s\"", retry_path->path);
        sentry__path_free(retry_path);
        return false;
    }

    sentry_uuid_t event_id = sentry__envelope_get_event_id(envelope);
    if (sentry_uuid_is_nil(&event_id)) {
        event_id = sentry_uuid_new_v4();
    }

    // Find existing retry file and get current attempt
    int current_attempt = find_retry_attempt(retry_path, &event_id);
    int next_attempt = current_attempt + 1;

    // Remove old retry file if it exists
    if (current_attempt > 0) {
        remove_retry_file(retry_path, &event_id);
    }

    // If max retries exceeded, move to cache or discard
    if (next_attempt > attempts) {
        sentry__path_free(retry_path);
        if (cache_keep) {
            SENTRY_WARNF(
                "max retry attempts (%d) exceeded, moving to cache", attempts);
            return write_to_cache(database_path, envelope);
        } else {
            SENTRY_WARNF(
                "max retry attempts (%d) exceeded, discarding", attempts);
            return false;
        }
    }

    // Write new retry file with incremented attempt
    char *filename = make_retry_filename(&event_id, next_attempt);
    if (!filename) {
        sentry__path_free(retry_path);
        return false;
    }

    sentry_path_t *output_path = sentry__path_join_str(retry_path, filename);
    sentry_free(filename);
    sentry__path_free(retry_path);

    if (!output_path) {
        return false;
    }

    int rv = sentry_envelope_write_to_path(envelope, output_path);
    sentry__path_free(output_path);
    if (rv) {
        SENTRY_WARN("writing envelope to retry file failed");
        return false;
    }

    SENTRY_DEBUGF(
        "wrote envelope to retry (attempt %d/%d)", next_attempt, attempts);
    return true;
}

void
sentry__retry_remove_envelope(
    const sentry_path_t *database_path, const sentry_uuid_t *envelope_id)
{
    if (!database_path || !envelope_id) {
        return;
    }

    sentry_path_t *retry_path = get_retry_path(database_path);
    if (!retry_path) {
        return;
    }

    remove_retry_file(retry_path, envelope_id);
    sentry__path_free(retry_path);
}

void
sentry__retry_cache_envelope(
    const sentry_path_t *database_path, const sentry_uuid_t *envelope_id)
{
    if (!database_path || !envelope_id) {
        return;
    }

    sentry_path_t *retry_path = get_retry_path(database_path);
    sentry_path_t *cache_path = get_cache_path(database_path);
    if (!retry_path || !cache_path) {
        sentry__path_free(retry_path);
        sentry__path_free(cache_path);
        return;
    }

    if (sentry__path_create_dir_all(cache_path) != 0) {
        sentry__path_free(retry_path);
        sentry__path_free(cache_path);
        return;
    }

    // Find the retry file (any attempt number)
    char uuid_str[37];
    sentry_uuid_as_string(envelope_id, uuid_str);

    sentry_pathiter_t *iter = sentry__path_iter_directory(retry_path);
    const sentry_path_t *file;
    while (iter && (file = sentry__pathiter_next(iter)) != NULL) {
        const char *filename = sentry__path_filename(file);
        if (!filename) {
            continue;
        }

        // Match <timestamp>-<counter>-<uuid>.envelope
        const char *first_dash = strchr(filename, '-');
        if (!first_dash) {
            continue;
        }
        const char *second_dash = strchr(first_dash + 1, '-');
        if (!second_dash) {
            continue;
        }
        const char *uuid_start = second_dash + 1;
        if (strncmp(uuid_start, uuid_str, 36) == 0
            && strcmp(uuid_start + 36, ".envelope") == 0) {
            char *cache_filename
                = sentry__uuid_as_filename(envelope_id, ".envelope");
            if (cache_filename) {
                sentry_path_t *dst
                    = sentry__path_join_str(cache_path, cache_filename);
                sentry_free(cache_filename);
                if (dst) {
                    sentry__path_rename(file, dst);
                    sentry__path_free(dst);
                }
            }
            break;
        }
    }
    sentry__pathiter_free(iter);
    sentry__path_free(retry_path);
    sentry__path_free(cache_path);
}

static int
compare_paths_by_filename(const void *a, const void *b)
{
    const sentry_path_t *pa = *(const sentry_path_t *const *)a;
    const sentry_path_t *pb = *(const sentry_path_t *const *)b;
    return strcmp(sentry__path_filename(pa), sentry__path_filename(pb));
}

void
sentry__retry_process_envelopes(const sentry_options_t *options)
{
    if (!options || !options->database_path) {
        return;
    }

    sentry_path_t *retry_path = get_retry_path(options->database_path);
    if (!retry_path) {
        return;
    }

    // Collect envelope paths
    size_t count = 0;
    size_t capacity = 8;
    sentry_path_t **paths = sentry_malloc(capacity * sizeof(sentry_path_t *));
    if (!paths) {
        sentry__path_free(retry_path);
        return;
    }

    sentry_pathiter_t *iter = sentry__path_iter_directory(retry_path);
    const sentry_path_t *file;
    while (iter && (file = sentry__pathiter_next(iter)) != NULL) {
        if (!sentry__path_ends_with(file, ".envelope")) {
            continue;
        }
        if (count == capacity) {
            capacity *= 2;
            sentry_path_t **tmp
                = sentry_malloc(capacity * sizeof(sentry_path_t *));
            if (!tmp) {
                break;
            }
            memcpy(tmp, paths, count * sizeof(sentry_path_t *));
            sentry_free(paths);
            paths = tmp;
        }
        paths[count] = sentry__path_clone(file);
        if (paths[count]) {
            count++;
        }
    }
    sentry__pathiter_free(iter);
    sentry__path_free(retry_path);

    // Sort by filename (timestamp prefix gives oldest-first ordering)
    if (count > 1) {
        qsort(paths, count, sizeof(sentry_path_t *), compare_paths_by_filename);
    }

    for (size_t i = 0; i < count; i++) {
        sentry_envelope_t *envelope = sentry__envelope_from_path(paths[i]);
        if (envelope) {
            SENTRY_DEBUG("retrying envelope from disk");
            if (!sentry__transport_retry_envelope(
                    options->transport, envelope)) {
                sentry_envelope_free(envelope);
            }
        } else {
            SENTRY_WARN("removing invalid envelope from retry directory");
            sentry__path_remove(paths[i]);
        }
        sentry__path_free(paths[i]);
    }
    sentry_free(paths);
}
