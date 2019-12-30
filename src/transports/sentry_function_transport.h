#ifndef SENTRY_TRANSPORTS_FUNCTION_TRANSPORT_H_INCLUDED
#define SENTRY_TRANSPORTS_FUNCTION_TRANSPORT_H_INCLUDED

#include <sentry.h>

sentry_transport_t *sentry__new_function_transport(
    void (*func)(sentry_envelope_t *envelope, void *data), void *data);

#endif