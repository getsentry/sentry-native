#include "sentry_http_transport.h"
#include "sentry_alloc.h"
#include "sentry_database.h"
#include "sentry_envelope.h"
#include "sentry_options.h"
#include "sentry_transport.h"

#include <string.h>

typedef struct {
    void *backend_state;
    void (*free_backend_state)(void *);
    int (*start_backend)(const sentry_options_t *, void *);
    sentry_task_exec_func_t send_task;
    void (*shutdown_hook)(void *backend_state);
} http_transport_state_t;

static void
http_transport_state_free(void *_state)
{
    http_transport_state_t *state = _state;
    if (state->free_backend_state) {
        state->free_backend_state(state->backend_state);
    }
    sentry_free(state);
}

static void
http_send_task(void *_envelope, void *_state)
{
    http_transport_state_t *state = _state;
    state->send_task(_envelope, state->backend_state);
}

static int
http_transport_start(const sentry_options_t *options, void *transport_state)
{
    sentry_bgworker_t *bgworker = transport_state;
    http_transport_state_t *state = sentry__bgworker_get_state(bgworker);

    sentry__bgworker_setname(bgworker, options->transport_thread_name);

    if (state->start_backend) {
        int rv = state->start_backend(options, state->backend_state);
        if (rv != 0) {
            return rv;
        }
    }

    return sentry__bgworker_start(bgworker);
}

static int
http_transport_flush(uint64_t timeout, void *transport_state)
{
    sentry_bgworker_t *bgworker = transport_state;
    return sentry__bgworker_flush(bgworker, timeout);
}

static int
http_transport_shutdown(uint64_t timeout, void *transport_state)
{
    sentry_bgworker_t *bgworker = transport_state;
    http_transport_state_t *state = sentry__bgworker_get_state(bgworker);

    int rv = sentry__bgworker_shutdown(bgworker, timeout);
    if (rv != 0 && state->shutdown_hook) {
        state->shutdown_hook(state->backend_state);
    }
    return rv;
}

static void
http_transport_send_envelope(sentry_envelope_t *envelope, void *transport_state)
{
    sentry_bgworker_t *bgworker = transport_state;
    sentry__bgworker_submit(bgworker, http_send_task,
        (void (*)(void *))sentry_envelope_free, envelope);
}

static bool
http_dump_task_cb(void *envelope, void *run)
{
    sentry__run_write_envelope(
        (sentry_run_t *)run, (sentry_envelope_t *)envelope);
    return true;
}

static size_t
http_dump_queue(sentry_run_t *run, void *transport_state)
{
    sentry_bgworker_t *bgworker = transport_state;
    return sentry__bgworker_foreach_matching(
        bgworker, http_send_task, http_dump_task_cb, run);
}

sentry_transport_t *
sentry__http_transport_new(void *backend_state,
    void (*free_backend_state)(void *),
    int (*start_backend)(const sentry_options_t *, void *),
    sentry_task_exec_func_t send_task,
    void (*shutdown_hook)(void *backend_state))
{
    http_transport_state_t *state = SENTRY_MAKE(http_transport_state_t);
    if (!state) {
        return NULL;
    }
    memset(state, 0, sizeof(http_transport_state_t));
    state->backend_state = backend_state;
    state->free_backend_state = free_backend_state;
    state->start_backend = start_backend;
    state->send_task = send_task;
    state->shutdown_hook = shutdown_hook;

    sentry_bgworker_t *bgworker
        = sentry__bgworker_new(state, http_transport_state_free);
    if (!bgworker) {
        http_transport_state_free(state);
        return NULL;
    }

    sentry_transport_t *transport
        = sentry_transport_new(http_transport_send_envelope);
    if (!transport) {
        sentry__bgworker_decref(bgworker);
        return NULL;
    }

    sentry_transport_set_state(transport, bgworker);
    sentry_transport_set_free_func(
        transport, (void (*)(void *))sentry__bgworker_decref);
    sentry_transport_set_startup_func(transport, http_transport_start);
    sentry_transport_set_flush_func(transport, http_transport_flush);
    sentry_transport_set_shutdown_func(transport, http_transport_shutdown);
    sentry__transport_set_dump_func(transport, http_dump_queue);

    return transport;
}
