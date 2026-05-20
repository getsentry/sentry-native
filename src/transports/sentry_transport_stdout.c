#include "sentry_core.h"
#include "sentry_transport.h"

#include <stdio.h>

static void
print_envelope(sentry_envelope_t *envelope, void *unused_state)
{
    (void)unused_state;
    size_t size_out = 0;
    char *serialized = sentry_envelope_serialize(envelope, &size_out);
    printf("%s", serialized);
    fflush(stdout);
    sentry_free(serialized);
    sentry_envelope_free(envelope);
}

sentry_transport_t *
sentry__transport_new_default(void)
{
    return sentry_transport_new(print_envelope);
}
