#ifndef SENTRY_TRANSPORTS_LIBCURL_TRANSPORT_H_INCLUDED
#define SENTRY_TRANSPORTS_LIBCURL_TRANSPORT_H_INCLUDED

#include <sentry.h>

sentry_transport_t *sentry__new_libcurl_transport(void);

#endif