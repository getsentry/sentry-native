#include "sentry_core.h"
#include "sentry_transport.h"

#include <stdio.h>

#ifdef SENTRY_PLATFORM_WINDOWS
#    include <fcntl.h>
#    include <io.h>
#endif

static void
print_envelope(sentry_envelope_t *envelope, void *unused_state)
{
    (void)unused_state;
    size_t size_out = 0;
    char *serialized = sentry_envelope_serialize(envelope, &size_out);
    if (serialized) {
        fwrite(serialized, 1, size_out, stdout);
        fflush(stdout);
    }
    sentry_free(serialized);
    sentry_envelope_free(envelope);
}

sentry_transport_t *
sentry_transport_new_default(void)
{
#ifdef SENTRY_PLATFORM_WINDOWS
    fflush(stdout);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    return sentry_transport_new(print_envelope);
}
