#ifndef SENTRY_HTTP_TRANSPORT_H_INCLUDED
#define SENTRY_HTTP_TRANSPORT_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_ratelimiter.h"
#include "sentry_sync.h"
#include "sentry_transport.h"

typedef void (*sentry_http_send_func_t)(void *client,
    sentry_prepared_http_request_t *req, sentry_rate_limiter_t *rl);

/**
 * Creates a new HTTP transport with the given client and send function.
 * Use the setter functions below to configure optional client callbacks.
 */
sentry_transport_t *sentry__http_transport_new(
    void *client, sentry_http_send_func_t send_func);

void sentry__http_transport_set_free_client(
    sentry_transport_t *transport, void (*free_client)(void *));
void sentry__http_transport_set_start_client(sentry_transport_t *transport,
    int (*start_client)(void *, const sentry_options_t *));
void sentry__http_transport_set_shutdown_client(
    sentry_transport_t *transport, void (*shutdown_client)(void *));

#ifdef SENTRY_UNITTEST
void *sentry__http_transport_get_bgworker(sentry_transport_t *transport);
#endif

#endif
