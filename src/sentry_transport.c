#include "sentry_boot.h"
#ifdef SENTRY_WITH_LIBCURL_TRANSPORT
#    include "transports/sentry_libcurl_transport.h"
#endif

sentry_transport_t *
sentry__transport_new_default(void)
{
#ifdef SENTRY_WITH_LIBCURL_TRANSPORT
    return sentry__new_libcurl_transport();
#endif
    return NULL;
}
