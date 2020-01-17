#include "sentry_disk_transport.h"
#include "../sentry_alloc.h"
#include "../sentry_core.h"
#include "../sentry_envelope.h"
#include "../sentry_path.h"
#include "../sentry_string.h"

static void
free_transport(sentry_transport_t *transport)
{
    sentry__path_free(transport->data);
}

static void
send_envelope(sentry_transport_t *transport, sentry_envelope_t *envelope)
{
    const sentry_path_t *database_path = transport->data;

    char event_id_str[37];
    sentry_uuid_t event_id = sentry__envelope_get_event_id(envelope);
    sentry_uuid_as_string(&event_id, event_id_str);

    sentry_path_t *output_path
        = sentry__path_join_str(database_path, event_id_str);

    sentry_envelope_write_to_path(envelope, output_path);
    sentry__path_free(output_path);
    sentry_envelope_free(envelope);
}

sentry_transport_t *
sentry_new_disk_transport(const sentry_path_t *database_path)
{
    sentry_transport_t *transport = SENTRY_MAKE(sentry_transport_t);
    if (!transport) {
        return NULL;
    }

    sentry_path_t *database_path_copy = sentry__path_clone(database_path);
    if (!database_path_copy) {
        sentry_free(transport);
        return NULL;
    }

    transport->data = database_path_copy;
    transport->free_func = free_transport;
    transport->send_envelope_func = send_envelope;
    transport->startup_func = NULL;
    transport->shutdown_func = NULL;

    return transport;
}