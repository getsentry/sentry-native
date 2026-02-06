#ifndef SENTRY_TRANSPORT_H_INCLUDED
#define SENTRY_TRANSPORT_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_database.h"
#include "sentry_ratelimiter.h"
#include "sentry_utils.h"

/**
 * Classification of HTTP send results per
 * https://develop.sentry.dev/sdk/expected-features/#buffer-to-disk
 */
typedef enum {
    SENTRY_SEND_SUCCESS, // 2xx — envelope accepted
    SENTRY_SEND_DISCARDED, // 4xx/5xx non-429 — envelope rejected
    SENTRY_SEND_RATE_LIMITED, // 429 — pause queue consumption
    SENTRY_SEND_NETWORK_ERROR // timeout, DNS, connection refused — retry later
} sentry_send_result_t;

typedef sentry_send_result_t (*sentry_transport_send_func_t)(
    void *envelope, void *state);

/**
 * Sets the dump function of the transport.
 *
 * This function is called during a hard crash to dump any internal send queue
 * to disk, using `sentry__run_write_envelope`. The function runs inside a
 * signal handler, and appropriate restrictions apply.
 */
void sentry__transport_set_dump_func(sentry_transport_t *transport,
    size_t (*dump_func)(sentry_run_t *run, void *state));

/**
 * Sets the synchronous send function used for retrying envelopes.
 * This is the raw send function (e.g. sentry__curl_send) that runs
 * directly on the bgworker thread.
 */
void sentry__transport_set_send_for_retry_func(
    sentry_transport_t *transport, sentry_transport_send_func_t send_func);

sentry_transport_send_func_t sentry__transport_get_send_for_retry_func(
    sentry_transport_t *transport);

/**
 * Submit the given envelope to the transport.
 */
void sentry__transport_send_envelope(
    sentry_transport_t *transport, sentry_envelope_t *envelope);

/**
 * Calls the transports startup hook.
 *
 * Returns 0 on success.
 */
int sentry__transport_startup(
    sentry_transport_t *transport, const sentry_options_t *options);

/**
 * Instructs the transport to flush its queue.
 *
 * Returns 0 on success.
 */
int sentry__transport_flush(sentry_transport_t *transport, uint64_t timeout);

/**
 * Instructs the transport to shut down.
 *
 * Returns 0 on success.
 */
int sentry__transport_shutdown(sentry_transport_t *transport, uint64_t timeout);

/**
 * This will create a new platform specific HTTP transport.
 */
sentry_transport_t *sentry__transport_new_default(void);

/**
 * This function will instruct the platform specific transport to dump all the
 * envelopes in its send queue to disk.
 */
size_t sentry__transport_dump_queue(
    sentry_transport_t *transport, sentry_run_t *run);

/**
 * Get the bgworker from an HTTP transport.
 */
void *sentry__transport_get_bgworker(sentry_transport_t *transport);

typedef struct sentry_prepared_http_header_s {
    const char *key;
    char *value;
} sentry_prepared_http_header_t;

/**
 * This represents a HTTP request, with method, url, headers and a body.
 */
typedef struct sentry_prepared_http_request_s {
    const char *method;
    char *url;
    sentry_prepared_http_header_t *headers;
    size_t headers_len;
    char *body;
    size_t body_len;
    bool body_owned;
} sentry_prepared_http_request_t;

/**
 * Consumes the given envelope and transforms it into into a prepared http
 * request. This can return NULL when all the items in the envelope have been
 * rate limited.
 */
sentry_prepared_http_request_t *sentry__prepare_http_request(
    sentry_envelope_t *envelope, const sentry_dsn_t *dsn,
    const sentry_rate_limiter_t *rl, const char *user_agent);

/**
 * Free a previously allocated HTTP request.
 */
void sentry__prepared_http_request_free(sentry_prepared_http_request_t *req);

#endif
