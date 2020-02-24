#include "sentry_function_transport.h"
#include "sentry_alloc.h"
#include "sentry_core.h"
#include "sentry_envelope.h"
#include "sentry_string.h"
#include "sentry_sync.h"

struct transport_state {
    void (*func)(sentry_envelope_t *envelope, void *data);
    void *data;
};

static void
free_transport(sentry_transport_t *transport)
{
    sentry_free(transport->data);
}

static void
send_envelope(struct sentry_transport_s *transport, sentry_envelope_t *envelope)
{
    struct transport_state *state = transport->data;
    state->func(envelope, state->data);
    sentry_envelope_free(envelope);
}

sentry_transport_t *
sentry_new_function_transport(
    void (*func)(sentry_envelope_t *envelope, void *data), void *data)
{
    SENTRY_DEBUG("initializing function transport");
    sentry_transport_t *transport = SENTRY_MAKE(sentry_transport_t);
    if (!transport) {
        return NULL;
    }

    struct transport_state *state = SENTRY_MAKE(struct transport_state);
    if (!state) {
        sentry_free(transport);
        return NULL;
    }

    state->func = func;
    state->data = data;
    transport->data = state;
    transport->free_func = free_transport;
    transport->send_envelope_func = send_envelope;
    transport->startup_func = NULL;
    transport->shutdown_func = NULL;

    return transport;
}
