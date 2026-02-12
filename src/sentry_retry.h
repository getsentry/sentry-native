#ifndef SENTRY_RETRY_H_INCLUDED
#define SENTRY_RETRY_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_path.h"

#define SENTRY_RETRY_BACKOFF_BASE_S 900
#define SENTRY_RETRY_STARTUP_DELAY_MS 100

typedef struct sentry_retry_s sentry_retry_t;

sentry_retry_t *sentry__retry_new(const sentry_options_t *options);
void sentry__retry_free(sentry_retry_t *retry);

void sentry__retry_write_envelope(
    sentry_retry_t *retry, const sentry_envelope_t *envelope);

void sentry__retry_set_startup_time(
    sentry_retry_t *retry, uint64_t startup_time);

size_t sentry__retry_foreach(sentry_retry_t *retry, bool startup,
    bool (*callback)(const sentry_path_t *path, void *data), void *data);

void sentry__retry_handle_result(
    sentry_retry_t *retry, const sentry_path_t *path, int status_code);

uint64_t sentry__retry_backoff(int count);

bool sentry__retry_parse_filename(const char *filename, uint64_t *ts_out,
    int *count_out, const char **uuid_out);

#endif
