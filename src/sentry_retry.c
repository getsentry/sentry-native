#include "sentry_retry.h"
#include "sentry_alloc.h"
#include "sentry_database.h"
#include "sentry_envelope.h"
#include "sentry_logger.h"
#include "sentry_options.h"
#include "sentry_transport.h"
#include "sentry_utils.h"
#include "sentry_uuid.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

sentry_retry_t *
sentry__retry_new(const sentry_options_t *options)
{
    if (!options || !options->run || options->http_retry <= 0) {
        return NULL;
    }

    sentry_retry_t *retry = SENTRY_MAKE(sentry_retry_t);
    if (!retry) {
        return NULL;
    }
    retry->run = options->run;
    retry->transport = options->transport;
    retry->max_attempts = options->http_retry;
    retry->cache_keep = options->cache_keep;
    return retry;
}

void
sentry__retry_free(sentry_retry_t *retry)
{
    if (!retry) {
        return;
    }
    sentry_free(retry);
}

static char *
make_retry_filename(const sentry_uuid_t *envelope_id, int attempt)
{
    char uuid_str[37];
    sentry_uuid_as_string(envelope_id, uuid_str);

    char *filename = sentry_malloc(80);
    if (filename) {
        snprintf(filename, 80, "%llu-%02d-%s.envelope",
            (unsigned long long)time(NULL), attempt, uuid_str);
    }
    return filename;
}

static bool
parse_retry_filename(
    const sentry_path_t *path, time_t *out_timestamp, int *out_attempt)
{
    const char *filename = sentry__path_filename(path);
    if (!filename) {
        return false;
    }
    char *end;
    unsigned long long ts = strtoull(filename, &end, 10);
    if (end == filename || *end != '-') {
        return false;
    }
    long attempt = strtol(end + 1, &end, 10);
    if (*end != '-' || attempt <= 0 || attempt > 99) {
        return false;
    }
    if (out_timestamp) {
        *out_timestamp = (time_t)ts;
    }
    if (out_attempt) {
        *out_attempt = (int)attempt;
    }
    return true;
}

static time_t
next_retry_time(const sentry_path_t *path)
{
    time_t timestamp;
    int attempt;
    if (!parse_retry_filename(path, &timestamp, &attempt) || attempt <= 1) {
        return 0;
    }
    // cap at 2h (base << 3)
    time_t delay = (time_t)SENTRY_RETRY_BASE_DELAY_S << MIN(attempt - 2, 3);
    return timestamp + delay;
}

static const sentry_path_t *
find_retry_file(sentry_pathiter_t *iter, const char *uuid_str)
{
    const sentry_path_t *file;
    while ((file = sentry__pathiter_next(iter)) != NULL) {
        const char *filename = sentry__path_filename(file);
        if (!filename) {
            continue;
        }
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
            return file;
        }
    }
    return NULL;
}

static bool
rename_retry_file(const sentry_path_t *retry_path, const sentry_path_t *old,
    const sentry_uuid_t *envelope_id, int next_attempt)
{
    char *filename = make_retry_filename(envelope_id, next_attempt);
    if (!filename) {
        return false;
    }
    sentry_path_t *dst = sentry__path_join_str(retry_path, filename);
    sentry_free(filename);
    if (!dst) {
        return false;
    }
    int rv = sentry__path_rename(old, dst);
    sentry__path_free(dst);
    return rv == 0;
}

static void
remove_retry_file(
    const sentry_path_t *retry_path, const sentry_uuid_t *envelope_id)
{
    char uuid_str[37];
    sentry_uuid_as_string(envelope_id, uuid_str);

    sentry_pathiter_t *iter = sentry__path_iter_directory(retry_path);
    const sentry_path_t *file = iter ? find_retry_file(iter, uuid_str) : NULL;
    if (file) {
        sentry__path_remove(file);
    }
    sentry__pathiter_free(iter);
}

static bool
retry_write_envelope(
    const sentry_retry_t *retry, const sentry_envelope_t *envelope)
{
    if (!retry || !envelope) {
        return false;
    }

    const sentry_path_t *retry_path = retry->run->retry_path;

    if (sentry__path_create_dir_all(retry_path) != 0) {
        SENTRY_ERRORF("mkdir failed: \"%s\"", retry_path->path);
        return false;
    }

    sentry_uuid_t event_id = sentry__envelope_get_event_id(envelope);
    if (sentry_uuid_is_nil(&event_id)) {
        event_id = sentry_uuid_new_v4();
    }

    int current_attempt = 0;
    sentry_path_t *existing = NULL;
    char uuid_str[37];
    sentry_uuid_as_string(&event_id, uuid_str);
    sentry_pathiter_t *iter = sentry__path_iter_directory(retry_path);
    const sentry_path_t *found = iter ? find_retry_file(iter, uuid_str) : NULL;
    if (found) {
        parse_retry_filename(found, NULL, &current_attempt);
        existing = sentry__path_clone(found);
    }
    sentry__pathiter_free(iter);

    int next_attempt = current_attempt + 1;

    if (next_attempt > retry->max_attempts) {
        if (existing) {
            sentry__path_remove(existing);
            sentry__path_free(existing);
        }
        if (retry->cache_keep) {
            SENTRY_WARNF("max retry attempts (%d) exceeded, moving to cache",
                retry->max_attempts);
            return sentry__run_write_cache(retry->run, envelope);
        } else {
            SENTRY_WARNF("max retry attempts (%d) exceeded, discarding",
                retry->max_attempts);
            return false;
        }
    }

    if (existing) {
        SENTRY_DEBUGF("renaming retry envelope (attempt %d/%d)", next_attempt,
            retry->max_attempts);
        bool rv
            = rename_retry_file(retry_path, existing, &event_id, next_attempt);
        sentry__path_free(existing);
        return rv;
    }

    char *filename = make_retry_filename(&event_id, next_attempt);
    if (!filename) {
        return false;
    }

    sentry_path_t *output_path = sentry__path_join_str(retry_path, filename);
    sentry_free(filename);

    if (!output_path) {
        return false;
    }

    int rv = sentry_envelope_write_to_path(envelope, output_path);
    sentry__path_free(output_path);
    if (rv) {
        SENTRY_WARN("writing envelope to retry file failed");
        return false;
    }

    SENTRY_DEBUGF("wrote envelope to retry (attempt %d/%d)", next_attempt,
        retry->max_attempts);
    return true;
}

static void
retry_cache_envelope(const sentry_path_t *retry_path, const sentry_run_t *run,
    const sentry_uuid_t *envelope_id)
{
    if (sentry__path_create_dir_all(run->cache_path) != 0) {
        return;
    }

    char uuid_str[37];
    sentry_uuid_as_string(envelope_id, uuid_str);

    sentry_pathiter_t *iter = sentry__path_iter_directory(retry_path);
    const sentry_path_t *file = iter ? find_retry_file(iter, uuid_str) : NULL;
    if (file) {
        char *cache_filename
            = sentry__uuid_as_filename(envelope_id, ".envelope");
        if (cache_filename) {
            sentry_path_t *dst
                = sentry__path_join_str(run->cache_path, cache_filename);
            sentry_free(cache_filename);
            if (dst) {
                sentry__path_rename(file, dst);
                sentry__path_free(dst);
            }
        }
    }
    sentry__pathiter_free(iter);
}

void
sentry__retry_process_result(sentry_retry_t *retry,
    const sentry_envelope_t *envelope, sentry_send_result_t result)
{
    if (!retry) {
        return;
    }

    if (result == SENTRY_SEND_NETWORK_ERROR) {
        retry_write_envelope(retry, envelope);
        return;
    }

    const sentry_path_t *retry_path = retry->run->retry_path;
    if (!sentry__path_is_dir(retry_path)) {
        return;
    }

    sentry_uuid_t event_id = sentry__envelope_get_event_id(envelope);

    switch (result) {
    case SENTRY_SEND_SUCCESS:
        if (retry->cache_keep) {
            retry_cache_envelope(retry_path, retry->run, &event_id);
        } else {
            remove_retry_file(retry_path, &event_id);
        }
        break;
    case SENTRY_SEND_RATE_LIMITED:
    case SENTRY_SEND_DISCARDED:
        remove_retry_file(retry_path, &event_id);
        break;
    default:
        break;
    }
}

static int
compare_paths_by_filename(const void *a, const void *b)
{
    const sentry_path_t *pa = *(const sentry_path_t *const *)a;
    const sentry_path_t *pb = *(const sentry_path_t *const *)b;
    return strcmp(sentry__path_filename(pa), sentry__path_filename(pb));
}

typedef struct {
    sentry_transport_t *transport;
    sentry_path_t **paths;
    size_t count;
    size_t index;
} retry_task_t;

static void retry_task_exec(void *_task, void *bgworker_state);
static void retry_rescan_exec(void *_retry, void *bgworker_state);

static void
retry_task_free(void *_task)
{
    retry_task_t *task = _task;
    for (size_t i = task->index; i < task->count; i++) {
        sentry__path_free(task->paths[i]);
    }
    sentry_free(task->paths);
    sentry_free(task);
}

static void
retry_task_exec(void *_task, void *bgworker_state)
{
    retry_task_t *task = _task;

    while (task->index < task->count) {
        sentry_path_t *path = task->paths[task->index];
        task->paths[task->index] = NULL;
        task->index++;

        sentry_envelope_t *envelope = sentry__envelope_from_path(path);
        sentry__path_free(path);

        if (!envelope) {
            SENTRY_WARN("removing invalid envelope from retry directory");
            continue;
        }

        SENTRY_DEBUG("retrying envelope from disk");
        sentry_send_result_t result = sentry__transport_send_retry(
            task->transport, envelope, bgworker_state);
        sentry_envelope_free(envelope);

        if (result == SENTRY_SEND_RATE_LIMITED
            || result == SENTRY_SEND_NETWORK_ERROR) {
            SENTRY_DEBUG(
                "stopping retry chain due to rate limit or network error");
            retry_task_free(task);
            return;
        }

        if (task->index < task->count) {
            if (sentry__transport_schedule_retry(task->transport,
                    retry_task_exec, NULL, task, SENTRY_RETRY_DELAY_MS)
                == 0) {
                return;
            }
        }
    }

    retry_task_free(task);
}

static void
retry_process_envelopes(sentry_retry_t *retry, bool check_backoff)
{
    if (!retry) {
        return;
    }

    const sentry_path_t *retry_path = retry->run->retry_path;
    time_t now = time(NULL);
    time_t earliest_pending = 0;

    size_t count = 0;
    size_t capacity = 8;
    sentry_path_t **paths = sentry_malloc(capacity * sizeof(sentry_path_t *));
    if (!paths) {
        return;
    }

    sentry_pathiter_t *iter = sentry__path_iter_directory(retry_path);
    const sentry_path_t *file;
    while (iter && (file = sentry__pathiter_next(iter)) != NULL) {
        if (!sentry__path_ends_with(file, ".envelope")) {
            continue;
        }
        if (check_backoff) {
            time_t ready_at = next_retry_time(file);
            if (ready_at > now) {
                if (earliest_pending == 0 || ready_at < earliest_pending) {
                    earliest_pending = ready_at;
                }
                continue;
            }
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

    if (count > 1) {
        qsort(paths, count, sizeof(sentry_path_t *), compare_paths_by_filename);
    }

    if (count == 0) {
        sentry_free(paths);
    } else {
        retry_task_t *task = sentry_malloc(sizeof(retry_task_t));
        if (!task) {
            for (size_t i = 0; i < count; i++) {
                sentry__path_free(paths[i]);
            }
            sentry_free(paths);
        } else {
            task->transport = retry->transport;
            task->paths = paths;
            task->count = count;
            task->index = 0;

            if (sentry__transport_schedule_retry(retry->transport,
                    retry_task_exec, NULL, task, SENTRY_RETRY_DELAY_MS)
                != 0) {
                retry_task_free(task);
            }
        }
    }

    if (earliest_pending > 0) {
        uint64_t delay_ms = (uint64_t)(earliest_pending - now) * 1000;
        sentry__transport_schedule_retry(
            retry->transport, retry_rescan_exec, NULL, retry, delay_ms);
    }
}

static void
retry_rescan_exec(void *_retry, void *bgworker_state)
{
    (void)bgworker_state;
    retry_process_envelopes(_retry, true);
}

void
sentry__retry_process_envelopes(sentry_retry_t *retry)
{
    retry_process_envelopes(retry, false);
}

void
sentry__retry_rescan_envelopes(sentry_retry_t *retry)
{
    retry_process_envelopes(retry, true);
}
