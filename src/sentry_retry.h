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

/**
 * Schedules retry polling on `bgworker` using `send_cb`.
 */
void sentry__retry_start(sentry_retry_t *retry, sentry_bgworker_t *bgworker,
    sentry_retry_send_func_t send_cb, void *send_data);

/**
 * Flushes unprocessed previous-session retries. No-op if already polled.
 */
void sentry__retry_flush(sentry_retry_t *retry, uint64_t timeout);

/**
 * Dumps queued envelopes to the retry dir and seals against further writes.
 */
void sentry__retry_dump_queue(
    sentry_retry_t *retry, sentry_task_exec_func_t task_func);

/**
 * Writes a failed envelope to the retry dir and schedules a delayed poll.
 */
void sentry__retry_enqueue(
    sentry_retry_t *retry, const sentry_envelope_t *envelope);

/**
 * Writes an event envelope to the retry dir. Non-event envelopes are skipped.
 */
void sentry__retry_write_envelope(
    sentry_retry_t *retry, const sentry_envelope_t *envelope);

/**
 * Sends eligible retry files via `send_cb`. `before > 0`: send files with
 * ts < before (startup). `before == 0`: use backoff. Returns remaining file
 * count for controlling polling.
 */
size_t sentry__retry_send(sentry_retry_t *retry, uint64_t before,
    sentry_retry_send_func_t send_cb, void *data);

/**
 * Exponential backoff: 15m, 30m, 1h, 2h, 4h, 8h, 8h, ... (capped at 8h).
 */
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

/**
 * Submits a delayed retry poll task on the background worker.
 */
void sentry__retry_trigger(sentry_retry_t *retry);

#endif
