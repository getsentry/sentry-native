#include "sentry_disk_transport.h"
#include "sentry_alloc.h"
#include "sentry_core.h"
#include "sentry_envelope.h"
#include "sentry_path.h"
#include "sentry_string.h"

static void
send_envelope(sentry_transport_t *transport, sentry_envelope_t *envelope)
{
    const sentry_run_t *run = transport->data;

    sentry__run_write_envelope(run, envelope);
    sentry_envelope_free(envelope);
}

sentry_transport_t *
sentry_new_disk_transport(const sentry_run_t *run)
{
    sentry_transport_t *transport = SENTRY_MAKE(sentry_transport_t);
    if (!transport) {
        return NULL;
    }

    transport->data = (void *)run;
    transport->free_func = NULL;
    transport->send_envelope_func = send_envelope;
    transport->startup_func = NULL;
    transport->shutdown_func = NULL;

    return transport;
}
