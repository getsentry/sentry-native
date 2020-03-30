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

// TODO: create proper platform dependent implementations
bool
sentry__path_is_locked(const sentry_path_t *path)
{
    return false;
}

sentry_filelock_t
sentry__path_lock(const sentry_path_t *path)
{
    sentry_filelock_t lock;
    lock._todo = NULL;
    return lock;
}

void
sentry__path_unlock(sentry_filelock_t lock)
{
}
