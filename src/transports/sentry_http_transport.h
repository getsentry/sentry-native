#ifndef SENTRY_HTTP_TRANSPORT_H_INCLUDED
#define SENTRY_HTTP_TRANSPORT_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_sync.h"

/**
 * Creates a new HTTP transport with the given backend.
 *
 * The transport manages bgworker lifecycle (start, flush, shutdown, dump)
 * and delegates actual HTTP sending to the backend's `send_task`.
 *
 * `shutdown_hook` is optional (NULL for curl). WinHTTP uses it to force-close
 * handles when bgworker_shutdown times out.
 */
sentry_transport_t *sentry__http_transport_new(void *backend_state,
    void (*free_backend_state)(void *),
    int (*start_backend)(const sentry_options_t *, void *),
    sentry_task_exec_func_t send_task,
    void (*shutdown_hook)(void *backend_state));

#endif
