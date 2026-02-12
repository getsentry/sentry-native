#include "sentry_retry.h"
#include "sentry_alloc.h"
#include "sentry_envelope.h"
#include "sentry_options.h"
#include "sentry_utils.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

struct sentry_retry_s {
    sentry_path_t *retry_dir;
    sentry_path_t *cache_dir;
    int max_retries;
    uint64_t startup_time;
};

sentry_retry_t *
sentry__retry_new(const sentry_options_t *options)
{
    if (options->http_retries <= 0 || !options->database_path) {
        return NULL;
    }
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

    const char *uuid_start = end + 1;
    size_t tail_len = strlen(uuid_start);
    // 36 chars UUID (with dashes) + ".envelope"
    if (tail_len != 36 + 9 || strcmp(uuid_start + 36, ".envelope") != 0) {
        return false;
    }

    *ts_out = ts;
    *count_out = (int)count;
    *uuid_out = uuid_start;
    return true;
}

uint64_t
sentry__retry_backoff(int count)
{
    int shift = count < 3 ? count : 3;
    return (uint64_t)SENTRY_RETRY_BACKOFF_BASE_S << shift;
}

static int
compare_retry_paths(const void *a, const void *b)
{
    const sentry_path_t *const *pa = a;
    const sentry_path_t *const *pb = b;
    return strcmp(sentry__path_filename(*pa), sentry__path_filename(*pb));
}

void
sentry__retry_write_envelope(
    sentry_retry_t *retry, const sentry_envelope_t *envelope)
{
    sentry_uuid_t event_id = sentry__envelope_get_event_id(envelope);
    if (sentry_uuid_is_nil(&event_id)) {
        return;
    }

    uint64_t now = (uint64_t)time(NULL);
    char uuid_str[37];
    sentry_uuid_as_string(&event_id, uuid_str);

    char filename[128];
    snprintf(filename, sizeof(filename), "%llu-00-%s.envelope",
        (unsigned long long)now, uuid_str);

    sentry_path_t *path = sentry__path_join_str(retry->retry_dir, filename);
    if (path) {
        (void)sentry_envelope_write_to_path(envelope, path);
        sentry__path_free(path);
    }
}

size_t
sentry__retry_foreach(sentry_retry_t *retry, bool startup,
    bool (*callback)(const sentry_path_t *path, void *data), void *data)
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

    size_t path_count = 0;
    uint64_t now = startup ? 0 : (uint64_t)time(NULL);

    const sentry_path_t *p;
    while ((p = sentry__pathiter_next(piter)) != NULL) {
        const char *fname = sentry__path_filename(p);
        uint64_t ts;
        int count;
        const char *uuid_start;
        if (!sentry__retry_parse_filename(fname, &ts, &count, &uuid_start)) {
            continue;
        }
        if (startup) {
            if (retry->startup_time > 0 && ts >= retry->startup_time) {
                continue;
            }
        } else if ((now - ts) < sentry__retry_backoff(count)) {
            continue;
        }
        if (path_count == path_cap) {
            path_cap *= 2;
            sentry_path_t **tmp
                = sentry_malloc(path_cap * sizeof(sentry_path_t *));
            if (!tmp) {
                break;
            }
            memcpy(tmp, paths, path_count * sizeof(sentry_path_t *));
            sentry_free(paths);
            paths = tmp;
        }
        paths[path_count++] = sentry__path_clone(p);
    }
    sentry__pathiter_free(piter);

    if (path_count > 1) {
        qsort(paths, path_count, sizeof(sentry_path_t *), compare_retry_paths);
    }

    for (size_t i = 0; i < path_count; i++) {
        if (!callback(paths[i], data)) {
            break;
        }
    }

    for (size_t i = 0; i < path_count; i++) {
        sentry__path_free(paths[i]);
    }
    sentry_free(paths);
    return path_count;
}

void
sentry__retry_handle_result(
    sentry_retry_t *retry, const sentry_path_t *path, int status_code)
{
    const char *fname = sentry__path_filename(path);
    uint64_t ts;
    int count;
    const char *uuid_start;
    if (!sentry__retry_parse_filename(fname, &ts, &count, &uuid_start)) {
        sentry__path_remove(path);
        return;
    }

    if (status_code < 0) {
        if (count + 1 >= retry->max_retries) {
            if (retry->cache_dir) {
                sentry_path_t *dst
                    = sentry__path_join_str(retry->cache_dir, fname);
                if (dst) {
                    sentry__path_rename(path, dst);
                    sentry__path_free(dst);
                } else {
                    sentry__path_remove(path);
                }
            } else {
                sentry__path_remove(path);
            }
        } else {
            uint64_t now = (uint64_t)time(NULL);
            char new_filename[128];
            snprintf(new_filename, sizeof(new_filename), "%llu-%02d-%s",
                (unsigned long long)now, count + 1, uuid_start);
            sentry_path_t *new_path
                = sentry__path_join_str(retry->retry_dir, new_filename);
            if (new_path) {
                sentry__path_rename(path, new_path);
                sentry__path_free(new_path);
            }
        }
    } else if (status_code >= 200 && status_code < 300) {
        if (retry->cache_dir) {
            sentry_path_t *dst = sentry__path_join_str(retry->cache_dir, fname);
            if (dst) {
                sentry__path_rename(path, dst);
                sentry__path_free(dst);
            } else {
                sentry__path_remove(path);
            }
        } else {
            sentry__path_remove(path);
        }
    } else {
        sentry__path_remove(path);
    }
}
