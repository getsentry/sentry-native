#ifndef SENTRY_RETRY_H_INCLUDED
#define SENTRY_RETRY_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_path.h"
#include "sentry_sync.h"

typedef struct sentry_retry_s sentry_retry_t;

typedef int (*sentry_retry_send_func_t)(
    sentry_envelope_t *envelope, void *data);

sentry_retry_t *sentry__retry_new(const sentry_options_t *options);
void sentry__retry_free(sentry_retry_t *retry);

void sentry__retry_start(sentry_retry_t *retry, sentry_bgworker_t *bgworker,
    sentry_retry_send_func_t send_cb, void *send_data);

void sentry__retry_flush(sentry_retry_t *retry, uint64_t timeout);

void sentry__retry_dump_queue(
    sentry_retry_t *retry, sentry_task_exec_func_t task_func);

void sentry__retry_enqueue(
    sentry_retry_t *retry, const sentry_envelope_t *envelope);

void sentry__retry_write_envelope(
    sentry_retry_t *retry, const sentry_envelope_t *envelope);

size_t sentry__retry_send(sentry_retry_t *retry, uint64_t before,
    sentry_retry_send_func_t send_cb, void *data);

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
