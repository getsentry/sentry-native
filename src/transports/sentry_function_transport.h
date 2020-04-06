#ifndef SENTRY_TRANSPORTS_FUNCTION_TRANSPORT_H_INCLUDED
#define SENTRY_TRANSPORTS_FUNCTION_TRANSPORT_H_INCLUDED

#include "sentry_boot.h"

/**
 * Create a new simple function transport.
 */
sentry_transport_t *sentry_new_function_transport(
    void (*func)(sentry_envelope_t *envelope, void *data), void *data);

#endif
