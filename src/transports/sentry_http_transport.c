#include "sentry_http_transport.h"
#include "sentry_alloc.h"
#include "sentry_database.h"
#include "sentry_envelope.h"
#include "sentry_options.h"
#include "sentry_string.h"
#include "sentry_transport.h"

#include <string.h>

typedef struct {
    sentry_dsn_t *dsn;
    char *user_agent;
    sentry_rate_limiter_t *ratelimiter;
    void *client;
    void (*free_client)(void *);
    int (*start_client)(const sentry_options_t *, void *);
    sentry_http_send_func_t send_func;
    void (*shutdown_hook)(void *client);
} http_transport_state_t;

static void
http_transport_state_free(void *_state)
{
    http_transport_state_t *state = _state;
    if (state->free_client) {
        state->free_client(state->client);
    }
    sentry__dsn_decref(state->dsn);
    sentry_free(state->user_agent);
    sentry__rate_limiter_free(state->ratelimiter);
    sentry_free(state);
}

static void
http_send_task(void *_envelope, void *_state)
{
    sentry_envelope_t *envelope = _envelope;
    http_transport_state_t *state = _state;

    sentry_prepared_http_request_t *req = sentry__prepare_http_request(
        envelope, state->dsn, state->ratelimiter, state->user_agent);
    if (!req) {
        return;
    }

    state->send_func(req, state->ratelimiter, state->client);
    sentry__prepared_http_request_free(req);
}

static int
http_transport_start(const sentry_options_t *options, void *transport_state)
{
    sentry_bgworker_t *bgworker = transport_state;
    http_transport_state_t *state = sentry__bgworker_get_state(bgworker);

    sentry__bgworker_setname(bgworker, options->transport_thread_name);

    state->dsn = sentry__dsn_incref(options->dsn);
    state->user_agent = sentry__string_clone(options->user_agent);

    if (state->start_client) {
        int rv = state->start_client(options, state->client);
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
        state->shutdown_hook(state->client);
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
sentry__http_transport_new(void *client, void (*free_client)(void *),
    int (*start_client)(const sentry_options_t *, void *),
    sentry_http_send_func_t send_func, void (*shutdown_hook)(void *client))
{
    http_transport_state_t *state = SENTRY_MAKE(http_transport_state_t);
    if (!state) {
        return NULL;
    }
    memset(state, 0, sizeof(http_transport_state_t));
    state->ratelimiter = sentry__rate_limiter_new();
    state->client = client;
    state->free_client = free_client;
    state->start_client = start_client;
    state->send_func = send_func;
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

#ifdef SENTRY_UNITTEST
void *
sentry__http_transport_get_bgworker(sentry_transport_t *transport)
{
    return sentry__transport_get_state(transport);
}
#endif
