#ifndef SENTRY_RETRY_H_INCLUDED
#define SENTRY_RETRY_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_path.h"
#include "sentry_sync.h"

typedef struct sentry_retry_s sentry_retry_t;

typedef bool (*sentry_retry_send_func_t)(const sentry_path_t *path, void *data);

sentry_retry_t *sentry__retry_new(const sentry_options_t *options);
void sentry__retry_free(sentry_retry_t *retry);

void sentry__retry_start(sentry_retry_t *retry, sentry_bgworker_t *bgworker,
    sentry_retry_send_func_t send_cb, void *send_data);

void sentry__retry_enqueue(
    sentry_retry_t *retry, const sentry_envelope_t *envelope);

void sentry__retry_write_envelope(
    sentry_retry_t *retry, const sentry_envelope_t *envelope);

size_t sentry__retry_foreach(sentry_retry_t *retry, uint64_t before,
    bool (*callback)(const sentry_path_t *path, void *data), void *data);

bool sentry__retry_handle_result(
    sentry_retry_t *retry, const sentry_path_t *path, int status_code);

uint64_t sentry__retry_backoff(int count);

/**
 * <db>/retry/<ts>-<count>-<uuid>.envelope
 */
sentry_path_t *sentry__retry_make_path(
    sentry_retry_t *retry, uint64_t ts, int count, const char *uuid);

/**
 * <ts>-<count>-<uuid>.envelope
 */
bool sentry__retry_parse_filename(const char *filename, uint64_t *ts_out,
    int *count_out, const char **uuid_out);

#endif
