#ifndef SENTRY_TRANSPORT_H_INCLUDED
#define SENTRY_TRANSPORT_H_INCLUDED

#include "sentry_boot.h"

typedef struct sentry_rate_limiter_s sentry_rate_limiter_t;

/**
 * The transport interface of sentry.
 */
// TODO: make this completely opaque, and rather use explicit functions from
// core
typedef struct sentry_transport_s {
    void (*send_envelope_func)(void *data, sentry_envelope_t *envelope);
    void (*startup_func)(void *data);
    bool (*shutdown_func)(void *data, uint64_t timeout);
    void (*free_func)(void *data);
    void *data;
} sentry_transport_t;

/**
 * This will create a new platform specific HTTP transport.
 */
sentry_transport_t *sentry__transport_new_default(void);

/**
 * This function will instruct the platform specific transport to dump all the
 * envelopes in its send queue to disk.
 */
void sentry__transport_dump_queue(sentry_transport_t *transport);

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
    sentry_envelope_t *envelope, const sentry_rate_limiter_t *rl);

/**
 * Free a previously allocated HTTP request.
 */
void sentry__prepared_http_request_free(sentry_prepared_http_request_t *req);

#endif
