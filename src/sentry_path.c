#include "sentry_path.h"

void
sentry__path_free(sentry_path_t *path)
{
    if (!path) {
        return;
    }
    sentry_free(path->path);
    sentry_free(path);
}

int
sentry__path_touch(const sentry_path_t *path)
{
    FILE *f = sentry__path_open(path, "a");
    if (f) {
        fclose(f);
        return 0;
    }
    return 1;
}