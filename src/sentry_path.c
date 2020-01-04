#include "sentry_path.h"
#include "sentry_alloc.h"

void
sentry__path_free(sentry_path_t *path)
{
    if (!path) {
        return;
    }
    sentry_free(path->path);
    sentry_free(path);
}
