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
 * Sets the retry envelope function of the transport.
 *
 * This function is called on startup to retry sending envelopes that were
 * persisted due to network errors. If set, the transport supports HTTP retry.
 */
void sentry__transport_set_retry_envelope_func(sentry_transport_t *transport,
    void (*retry_envelope_func)(sentry_envelope_t *envelope, void *state,
        void (*on_result)(sentry_send_result_t, void *), void *user_data));

/**
 * Retry sending an envelope via the transport.
 *
 * Returns true if the transport supports retry and the envelope was submitted,
 * false if the transport does not support retry. Pass NULL envelope to check
 * capability without sending. The on_result callback is invoked on the worker
 * thread after the send completes with the send result.
 */
bool sentry__transport_retry_envelope(sentry_transport_t *transport,
    sentry_envelope_t *envelope,
    void (*on_result)(sentry_send_result_t, void *), void *user_data);

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

#ifdef SENTRY_UNITTEST
/**
 * Test helper function to get the bgworker from a transport.
 * Only available in unit tests and only works for HTTP transports.
 */
void *sentry__transport_get_bgworker(sentry_transport_t *transport);
#endif

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
