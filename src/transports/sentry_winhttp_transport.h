#ifndef SENTRY_TRANSPORTS_WINHTTP_TRANSPORT_H_INCLUDED
#define SENTRY_TRANSPORTS_WINHTTP_TRANSPORT_H_INCLUDED

#include "sentry_boot.h"

sentry_transport_t *sentry__new_winhttp_transport(void);

#endif
