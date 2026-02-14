#include "sentry_retry.h"
#include "sentry_alloc.h"
#include "sentry_envelope.h"
#include "sentry_options.h"
#include "sentry_utils.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SENTRY_RETRY_INTERVAL (15 * 60 * 1000)
#define SENTRY_RETRY_THROTTLE 100

struct sentry_retry_s {
    sentry_path_t *retry_dir;
    sentry_path_t *cache_dir;
    int max_retries;
    uint64_t startup_time;
    sentry_bgworker_t *bgworker;
    sentry_retry_send_func_t send_cb;
    void *send_data;
};

sentry_retry_t *
sentry__retry_new(const sentry_options_t *options)
{
    sentry_path_t *retry_dir
        = sentry__path_join_str(options->database_path, "retry");
    if (!retry_dir) {
        return NULL;
    }
    sentry_path_t *cache_dir = NULL;
    if (options->cache_keep) {
        cache_dir = sentry__path_join_str(options->database_path, "cache");
    }

    sentry_retry_t *retry = SENTRY_MAKE(sentry_retry_t);
    if (!retry) {
        sentry__path_free(cache_dir);
        sentry__path_free(retry_dir);
        return NULL;
    }
    retry->retry_dir = retry_dir;
    retry->cache_dir = cache_dir;
    retry->max_retries = options->http_retries;
    retry->startup_time = (uint64_t)time(NULL);
    sentry__path_create_dir_all(retry->retry_dir);
    if (retry->cache_dir) {
        sentry__path_create_dir_all(retry->cache_dir);
    }
    return retry;
}

void
sentry__retry_free(sentry_retry_t *retry)
{
    if (!retry) {
        return;
    }
    sentry__path_free(retry->retry_dir);
    sentry__path_free(retry->cache_dir);
    sentry_free(retry);
}

bool
sentry__retry_parse_filename(const char *filename, uint64_t *ts_out,
    int *count_out, const char **uuid_out)
{
    char *end;
    uint64_t ts = strtoull(filename, &end, 10);
    if (*end != '-') {
        return false;
    }

    const char *count_str = end + 1;
    long count = strtol(count_str, &end, 10);
    if (*end != '-') {
        return false;
    }

    const char *uuid = end + 1;
    size_t tail_len = strlen(uuid);
    // 36 chars UUID (with dashes) + ".envelope"
    if (tail_len != 36 + 9 || strcmp(uuid + 36, ".envelope") != 0) {
        return false;
    }

    *ts_out = ts;
    *count_out = (int)count;
    *uuid_out = uuid;
    return true;
}

uint64_t
sentry__retry_backoff(int count)
{
    int shift = count < 3 ? count : 3;
    return (uint64_t)(SENTRY_RETRY_INTERVAL / 1000) << shift;
}

static int
compare_retry_paths(const void *a, const void *b)
{
    const sentry_path_t *const *pa = a;
    const sentry_path_t *const *pb = b;
    return strcmp(sentry__path_filename(*pa), sentry__path_filename(*pb));
}

sentry_path_t *
sentry__retry_make_path(
    sentry_retry_t *retry, uint64_t ts, int count, const char *uuid)
{
    char filename[128];
    snprintf(filename, sizeof(filename), "%" PRIu64 "-%02d-%.36s.envelope", ts,
        count, uuid);
    return sentry__path_join_str(retry->retry_dir, filename);
}

void
sentry__retry_write_envelope(
    sentry_retry_t *retry, const sentry_envelope_t *envelope)
{
    sentry_uuid_t event_id = sentry__envelope_get_event_id(envelope);
    if (sentry_uuid_is_nil(&event_id)) {
        return;
    }

    char uuid[37];
    sentry_uuid_as_string(&event_id, uuid);

    sentry_path_t *path
        = sentry__retry_make_path(retry, (uint64_t)time(NULL), 0, uuid);
    if (path) {
        (void)sentry_envelope_write_to_path(envelope, path);
        sentry__path_free(path);
    }
}

static bool
handle_result(sentry_retry_t *retry, const sentry_path_t *path, int status_code)
{
    const char *fname = sentry__path_filename(path);
    uint64_t ts;
    int count;
    const char *uuid;
    if (!sentry__retry_parse_filename(fname, &ts, &count, &uuid)) {
        sentry__path_remove(path);
        return false;
    }

    if (status_code < 0 && count + 1 < retry->max_retries) {
        sentry_path_t *new_path = sentry__retry_make_path(
            retry, (uint64_t)time(NULL), count + 1, uuid);
        if (new_path) {
            sentry__path_rename(path, new_path);
            sentry__path_free(new_path);
        }
        return true;
    }

    if (count + 1 >= retry->max_retries && retry->cache_dir) {
        char cache_name[46];
        snprintf(cache_name, sizeof(cache_name), "%.36s.envelope", uuid);
        sentry_path_t *dst
            = sentry__path_join_str(retry->cache_dir, cache_name);
        if (dst) {
            sentry__path_rename(path, dst);
            sentry__path_free(dst);
        } else {
            sentry__path_remove(path);
        }
    } else {
        sentry__path_remove(path);
    }
    return false;
}

size_t
sentry__retry_send(sentry_retry_t *retry, uint64_t before,
    sentry_retry_send_func_t send_cb, void *data)
{
    sentry_pathiter_t *piter = sentry__path_iter_directory(retry->retry_dir);
    if (!piter) {
        return 0;
    }

    size_t path_cap = 16;
    sentry_path_t **paths = sentry_malloc(path_cap * sizeof(sentry_path_t *));
    if (!paths) {
        sentry__pathiter_free(piter);
        return 0;
    }

    size_t total = 0;
    size_t eligible = 0;
    uint64_t now = before > 0 ? 0 : (uint64_t)time(NULL);

    const sentry_path_t *p;
    while ((p = sentry__pathiter_next(piter)) != NULL) {
        const char *fname = sentry__path_filename(p);
        uint64_t ts;
        int count;
        const char *uuid;
        if (!sentry__retry_parse_filename(fname, &ts, &count, &uuid)) {
            continue;
        }
        if (before > 0 && ts >= before) {
            continue;
        }
        total++;
        if (!before && now >= ts && (now - ts) < sentry__retry_backoff(count)) {
            continue;
        }
        if (eligible == path_cap) {
            path_cap *= 2;
            sentry_path_t **tmp
                = sentry_malloc(path_cap * sizeof(sentry_path_t *));
            if (!tmp) {
                break;
            }
            memcpy(tmp, paths, eligible * sizeof(sentry_path_t *));
            sentry_free(paths);
            paths = tmp;
        }
        paths[eligible++] = sentry__path_clone(p);
    }
    sentry__pathiter_free(piter);

    if (eligible > 1) {
        qsort(paths, eligible, sizeof(sentry_path_t *), compare_retry_paths);
    }

    for (size_t i = 0; i < eligible; i++) {
        sentry_envelope_t *envelope = sentry__envelope_from_path(paths[i]);
        if (!envelope) {
            sentry__path_remove(paths[i]);
        } else {
            int status_code = send_cb(envelope, data);
            sentry_envelope_free(envelope);
            if (!handle_result(retry, paths[i], status_code)) {
                total--;
            }
        }
    }

    for (size_t i = 0; i < eligible; i++) {
        sentry__path_free(paths[i]);
    }
    sentry_free(paths);
    return total;
}

static void
retry_poll_task(void *_retry, void *_state)
{
    (void)_state;
    sentry_retry_t *retry = _retry;
    if (sentry__retry_send(
            retry, retry->startup_time, retry->send_cb, retry->send_data)) {
        sentry__bgworker_submit_delayed(retry->bgworker, retry_poll_task, NULL,
            retry, SENTRY_RETRY_INTERVAL);
    }
    // subsequent polls use backoff instead of the startup time filter
    retry->startup_time = 0;
}

void
sentry__retry_start(sentry_retry_t *retry, sentry_bgworker_t *bgworker,
    sentry_retry_send_func_t send_cb, void *send_data)
{
    retry->bgworker = bgworker;
    retry->send_cb = send_cb;
    retry->send_data = send_data;
    sentry__bgworker_submit_delayed(
        bgworker, retry_poll_task, NULL, retry, SENTRY_RETRY_THROTTLE);
}

static void
retry_flush_task(void *_retry, void *_state)
{
    (void)_state;
    sentry_retry_t *retry = _retry;
    if (retry->startup_time > 0) {
        sentry__retry_send(retry, UINT64_MAX, retry->send_cb, retry->send_data);
        retry->startup_time = 0;
    }
}

void
sentry__retry_flush(sentry_retry_t *retry)
{
    if (retry) {
        sentry__bgworker_submit(retry->bgworker, retry_flush_task, NULL, retry);
    }
}

void
sentry__retry_enqueue(sentry_retry_t *retry, const sentry_envelope_t *envelope)
{
    sentry__retry_write_envelope(retry, envelope);
    // prevent the startup poll from re-processing this session's envelope
    retry->startup_time = 0;
    sentry__bgworker_submit_delayed(
        retry->bgworker, retry_poll_task, NULL, retry, SENTRY_RETRY_INTERVAL);
}
