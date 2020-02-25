#include "sentry_backend.h"
#ifdef SENTRY_WITH_INPROC_BACKEND
#    include "backends/sentry_inproc_backend.h"
#endif
#ifdef SENTRY_WITH_CRASHPAD_BACKEND
#    include "backends/sentry_crashpad_backend.h"
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
#ifdef SENTRY_WITH_CRASHPAD_BACKEND
    return sentry__new_crashpad_backend();
#elif defined(SENTRY_WITH_INPROC_BACKEND)
    return sentry__new_inproc_backend();
#else
    return NULL;
#endif
}
