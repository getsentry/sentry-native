#include <sentry.h>

void
sentry_envelope_free(sentry_envelope_t *envelope)
{
    sentry_free(envelope);
}