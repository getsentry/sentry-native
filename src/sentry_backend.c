#include "sentry_backend.h"
#if SENTRY_PLATFORM != SENTRY_PLATFORM_WINDOWS
#    include "backends/sentry_inproc_backend.h"
#endif

void
sentry__backend_free(sentry_backend_t *backend)
{
    if (!backend) {
        return;
    }
    if (backend->free_func) {
        backend->free_func(backend);
    }
    sentry_free(backend);
}

sentry_backend_t *
sentry__backend_new_default(void)
{
#if SENTRY_PLATFORM != SENTRY_PLATFORM_WINDOWS
    return sentry__new_inproc_backend();
#else
    return NULL;
#endif
}