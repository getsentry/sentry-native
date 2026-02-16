#include "sentry_retry.h"
#include "sentry_alloc.h"
#include "sentry_database.h"
#include "sentry_envelope.h"
#include "sentry_logger.h"
#include "sentry_options.h"
#include "sentry_utils.h"

#include <stdlib.h>
#include <string.h>

#define SENTRY_RETRY_ATTEMPTS 5
#define SENTRY_RETRY_INTERVAL (15 * 60 * 1000)
#define SENTRY_RETRY_THROTTLE 100

struct sentry_retry_s {
    sentry_path_t *cache_path;
    bool cache_keep;
    uint64_t startup_time;
    volatile long sealed;
    sentry_bgworker_t *bgworker;
    sentry_retry_send_func_t send_cb;
    void *send_data;
};

sentry_retry_t *
sentry__retry_new(const sentry_options_t *options)
{
    sentry_retry_t *retry = SENTRY_MAKE(sentry_retry_t);
    if (!retry) {
        return NULL;
    }
    retry->cache_path = sentry__path_clone(options->run->cache_path);
    retry->cache_keep = options->cache_keep;
    retry->startup_time = sentry__usec_time() / 1000;
    retry->sealed = 0;
    sentry__path_create_dir_all(options->run->cache_path);
    return retry;
}

void
sentry__retry_free(sentry_retry_t *retry)
{
    if (!retry) {
        return;
    }
    sentry__path_free(retry->cache_path);
    sentry_free(retry);
}

bool
sentry__retry_parse_filename(const char *filename, uint64_t *ts_out,
    int *count_out, const char **uuid_out)
{
    // Minimum retry filename: <ts>-<count>-<uuid>.envelope (49+ chars).
    // Cache filenames are exactly 45 chars (<uuid>.envelope).
    if (strlen(filename) <= 45) {
        return false;
    }

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
    return (uint64_t)SENTRY_RETRY_INTERVAL << MIN(count, 5);
}

typedef struct {
    sentry_path_t *path;
    uint64_t ts;
    int count;
    char uuid[37];
} retry_item_t;

static int
compare_retry_items(const void *a, const void *b)
{
    const retry_item_t *ia = a;
    const retry_item_t *ib = b;
    if (ia->ts != ib->ts) {
        return ia->ts < ib->ts ? -1 : 1;
    }
    if (ia->count != ib->count) {
        return ia->count - ib->count;
    }
    return strcmp(ia->uuid, ib->uuid);
}

sentry_path_t *
sentry__retry_make_path(
    sentry_retry_t *retry, uint64_t ts, int count, const char *uuid)
{
    char filename[128];
    snprintf(filename, sizeof(filename), "%" PRIu64 "-%02d-%.36s.envelope", ts,
        count, uuid);
    return sentry__path_join_str(retry->cache_path, filename);
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
        = sentry__retry_make_path(retry, sentry__usec_time() / 1000, 0, uuid);
    if (path) {
        if (sentry_envelope_write_to_path(envelope, path) != 0) {
            SENTRY_WARNF(
                "failed to write retry envelope to \"%s\"", path->path);
        }
        sentry__path_free(path);
    }
}

static bool
handle_result(sentry_retry_t *retry, const retry_item_t *item, int status_code)
{
    // network failure with retries remaining: bump count & re-enqueue
    if (item->count + 1 < SENTRY_RETRY_ATTEMPTS && status_code < 0) {
        sentry_path_t *new_path = sentry__retry_make_path(
            retry, sentry__usec_time() / 1000, item->count + 1, item->uuid);
        if (new_path) {
            sentry__path_rename(item->path, new_path);
            sentry__path_free(new_path);
        }
        return true;
    }

    bool exhausted = item->count + 1 >= SENTRY_RETRY_ATTEMPTS;

    // network failure with retries exhausted
    if (exhausted && status_code < 0) {
        if (retry->cache_keep) {
            SENTRY_WARNF("max retries (%d) reached, moving envelope to cache",
                SENTRY_RETRY_ATTEMPTS);
        } else {
            SENTRY_WARNF("max retries (%d) reached, discarding envelope",
                SENTRY_RETRY_ATTEMPTS);
        }
    }

    // cache on last attempt
    if (exhausted && retry->cache_keep) {
        char cache_name[46];
        snprintf(cache_name, sizeof(cache_name), "%.36s.envelope", item->uuid);
        sentry_path_t *dest
            = sentry__path_join_str(retry->cache_path, cache_name);
        if (!dest || sentry__path_rename(item->path, dest) != 0) {
            sentry__path_remove(item->path);
        }
        sentry__path_free(dest);
        return false;
    }

    sentry__path_remove(item->path);
    return false;
}

size_t
sentry__retry_send(sentry_retry_t *retry, uint64_t before,
    sentry_retry_send_func_t send_cb, void *data)
{
    sentry_pathiter_t *piter = sentry__path_iter_directory(retry->cache_path);
    if (!piter) {
        return 0;
    }

    size_t item_cap = 16;
    retry_item_t *items = sentry_malloc(item_cap * sizeof(retry_item_t));
    if (!items) {
        sentry__pathiter_free(piter);
        return 0;
    }

    size_t total = 0;
    size_t eligible = 0;
    uint64_t now = before > 0 ? 0 : sentry__usec_time() / 1000;

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
        if (eligible == item_cap) {
            item_cap *= 2;
            retry_item_t *tmp = sentry_malloc(item_cap * sizeof(retry_item_t));
            if (!tmp) {
                break;
            }
            memcpy(tmp, items, eligible * sizeof(retry_item_t));
            sentry_free(items);
            items = tmp;
        }
        retry_item_t *item = &items[eligible++];
        item->path = sentry__path_clone(p);
        item->ts = ts;
        item->count = count;
        memcpy(item->uuid, uuid, 36);
        item->uuid[36] = '\0';
    }
    sentry__pathiter_free(piter);

    if (eligible > 1) {
        qsort(items, eligible, sizeof(retry_item_t), compare_retry_items);
    }

    for (size_t i = 0; i < eligible; i++) {
        sentry_envelope_t *envelope = sentry__envelope_from_path(items[i].path);
        if (!envelope) {
            sentry__path_remove(items[i].path);
            total--;
        } else {
            SENTRY_DEBUGF("retrying envelope (%d/%d)", items[i].count + 1,
                SENTRY_RETRY_ATTEMPTS);
            int status_code = send_cb(envelope, data);
            sentry_envelope_free(envelope);
            if (!handle_result(retry, &items[i], status_code)) {
                total--;
            }
            // stop on network failure to avoid wasting time on a dead
            // connection; remaining envelopes stay untouched for later
            if (status_code < 0) {
                break;
            }
        }
    }

    for (size_t i = 0; i < eligible; i++) {
        sentry__path_free(items[i].path);
    }
    sentry_free(items);
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
sentry__retry_flush(sentry_retry_t *retry, uint64_t timeout)
{
    if (retry) {
        sentry__bgworker_submit(retry->bgworker, retry_flush_task, NULL, retry);
        sentry__bgworker_flush(retry->bgworker, timeout);
    }
}

static bool
retry_dump_cb(void *_envelope, void *_retry)
{
    sentry__retry_write_envelope(
        (sentry_retry_t *)_retry, (sentry_envelope_t *)_envelope);
    return true;
}

void
sentry__retry_dump_queue(
    sentry_retry_t *retry, sentry_task_exec_func_t task_func)
{
    if (retry) {
        sentry__bgworker_foreach_matching(
            retry->bgworker, task_func, retry_dump_cb, retry);
        // prevent duplicate writes from a still-running detached worker
        sentry__atomic_store(&retry->sealed, 1);
    }
}

static void
retry_trigger_task(void *_retry, void *_state)
{
    (void)_state;
    sentry_retry_t *retry = _retry;
    if (sentry__retry_send(
            retry, UINT64_MAX, retry->send_cb, retry->send_data)) {
        sentry__retry_trigger(retry);
    }
}

void
sentry__retry_trigger(sentry_retry_t *retry)
{
    sentry__bgworker_submit(retry->bgworker, retry_trigger_task, NULL, retry);
}

void
sentry__retry_enqueue(sentry_retry_t *retry, const sentry_envelope_t *envelope)
{
    if (sentry__atomic_fetch(&retry->sealed)) {
        return;
    }
    sentry__retry_write_envelope(retry, envelope);
    // prevent the startup poll from re-processing this session's envelope
    retry->startup_time = 0;
    sentry__bgworker_submit_delayed(
        retry->bgworker, retry_poll_task, NULL, retry, SENTRY_RETRY_INTERVAL);
}
