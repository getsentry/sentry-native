#ifndef SENTRY_HTTP_TRANSPORT_H_INCLUDED
#define SENTRY_HTTP_TRANSPORT_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_ratelimiter.h"
#include "sentry_sync.h"
#include "sentry_transport.h"

typedef void (*sentry_http_send_func_t)(sentry_prepared_http_request_t *req,
    sentry_rate_limiter_t *rl, void *client);

/**
 * Creates a new HTTP transport with the given client.
 *
 * The transport manages bgworker lifecycle (start, flush, shutdown, dump)
 * and delegates actual HTTP sending to the client's `send_func`.
 *
 * `shutdown_hook` is optional (NULL for curl). WinHTTP uses it to force-close
 * handles when bgworker_shutdown times out.
 */
sentry_transport_t *sentry__http_transport_new(void *client,
    void (*free_client)(void *),
    int (*start_client)(const sentry_options_t *, void *),
    sentry_http_send_func_t send_func, void (*shutdown_hook)(void *client));

#ifdef SENTRY_UNITTEST
void *sentry__http_transport_get_bgworker(sentry_transport_t *transport);
#endif

#endif
