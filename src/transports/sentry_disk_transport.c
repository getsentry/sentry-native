#include "sentry_disk_transport.h"
#include "sentry_alloc.h"
#include "sentry_core.h"
#include "sentry_database.h"
#include "sentry_envelope.h"
#include "sentry_path.h"
#include "sentry_string.h"

static void
send_envelope(void *data, sentry_envelope_t *envelope)
{
    const sentry_run_t *run = data;

    sentry__run_write_envelope(run, envelope);
    sentry_envelope_free(envelope);
}

sentry_transport_t *
sentry_new_disk_transport(const sentry_run_t *run)
{
    return sentry_transport_new(send_envelope, (void *)run);
}
