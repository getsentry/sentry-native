#include "sentry_libcurl_transport.h"
#include "../sentry_alloc.h"
#include "../sentry_sync.h"
#include <curl/curl.h>
#include <curl/easy.h>

struct transport_state {
    bool initialized;
    CURL *curl_handle;
    uint64_t disabled_until;
    sentry_bgworker_t *bgworker;
};

struct task_state {
    struct transport_state *transport_state;
    sentry_envelope_t *envelope;
};

static struct transport_state *
new_transport_state(void)
{
    static bool curl_initialized = false;

    struct transport_state *state = SENTRY_MAKE(struct transport_state);
    if (!state) {
        return NULL;
    }

    state->bgworker = sentry__bgworker_new();
    if (!state->bgworker) {
        sentry_free(state);
        return NULL;
    }

    if (!curl_initialized) {
        curl_global_init(CURL_GLOBAL_ALL);
    }

    state->initialized = false;
    state->curl_handle = curl_easy_init();
    state->disabled_until = 0;

    return state;
}

static void
start_transport(sentry_transport_t *transport)
{
    struct transport_state *state = transport->data;
    sentry__bgworker_start(state->bgworker);
}

static void
shutdown_transport(sentry_transport_t *transport)
{
    struct transport_state *state = transport->data;
    sentry__bgworker_shutdown(state->bgworker, 5000);
}

static void
free_transport(sentry_transport_t *transport)
{
    struct transport_state *state = transport->data;
    curl_easy_cleanup(state->curl_handle);
    sentry__bgworker_free(state->bgworker);
    sentry_free(state);
}

static void
task_exec_func(void *data)
{
    struct task_state *state = data;
}

static void
task_cleanup_func(void *data)
{
    struct task_state *ts = data;
    sentry_envelope_free(ts->envelope);
    sentry_free(ts);
}

void
send_envelope(struct sentry_transport_s *transport, sentry_envelope_t *envelope)
{
    struct transport_state *state = transport->data;
    struct task_state *ts = SENTRY_MAKE(struct task_state);
    if (!ts) {
        sentry_envelope_free(envelope);
        return;
    }
    ts->transport_state = state;
    ts->envelope = envelope;
    sentry__bgworker_submit(
        state->bgworker, task_exec_func, task_cleanup_func, ts);
}

sentry_transport_t *
sentry__libcurl_new_transport(void)
{
    sentry_transport_t *transport = SENTRY_MAKE(sentry_transport_t);
    if (!transport) {
        return NULL;
    }

    struct transport_state *state = new_transport_state();
    if (!state) {
        sentry_free(transport);
        return NULL;
    }

    transport->data = state;
    transport->free_func = free_transport;
    transport->send_envelope_func = send_envelope;
    transport->startup_func = start_transport;
    transport->shutdown_func = shutdown_transport;

    return transport;
}