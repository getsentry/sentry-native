#ifndef SENTRY_RETRY_H_INCLUDED
#define SENTRY_RETRY_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_transport.h"

#define SENTRY_RETRY_THROTTLE 100
#define SENTRY_RETRY_INTERVAL (15 * 60 * 1000)

typedef struct {
    const struct sentry_run_s *run;
    sentry_transport_t *transport;
    int max_retries;
    bool cache_keep;
} sentry_retry_t;

sentry_retry_t *sentry__retry_new(const sentry_options_t *options);

void sentry__retry_free(sentry_retry_t *retry);

void sentry__retry_process_result(sentry_retry_t *retry,
    const sentry_envelope_t *envelope, sentry_send_result_t result);

void sentry__retry_process_envelopes(sentry_retry_t *retry);

void sentry__retry_rescan_envelopes(sentry_retry_t *retry);

#endif
