#ifndef SENTRY_RETRY_H_INCLUDED
#define SENTRY_RETRY_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_path.h"

#define SENTRY_RETRY_BACKOFF_BASE_S 900
#define SENTRY_RETRY_STARTUP_DELAY_MS 100

typedef struct sentry_retry_s sentry_retry_t;

sentry_retry_t *sentry__retry_new(
    sentry_path_t *retry_dir, sentry_path_t *cache_dir, int max_retries);
void sentry__retry_free(sentry_retry_t *retry);

void sentry__retry_write_envelope(
    sentry_retry_t *retry, const sentry_envelope_t *envelope);

void sentry__retry_set_startup_time(
    sentry_retry_t *retry, uint64_t startup_time);

sentry_path_t **sentry__retry_scan(
    sentry_retry_t *retry, bool startup, size_t *count_out);
void sentry__retry_free_paths(sentry_path_t **paths, size_t count);

void sentry__retry_handle_result(
    sentry_retry_t *retry, const sentry_path_t *path, int status_code);

bool sentry__retry_has_files(const sentry_retry_t *retry);

uint64_t sentry__retry_backoff(int count);

bool sentry__retry_parse_filename(const char *filename, uint64_t *ts_out,
    int *count_out, const char **uuid_out);

#endif
