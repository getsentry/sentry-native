#include "sentry_path.h"
#include "sentry_alloc.h"

/* only read this many bytes to memory ever */
const size_t MAX_READ_TO_BUFFER = 134217728;

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
    /* TODO: make async safe */
    FILE *f = sentry__path_open(path, "a");
    if (f) {
        fclose(f);
        return 0;
    }
    return 1;
}

char *
sentry__path_read_to_buffer(const sentry_path_t *path, size_t *size_out)
{
    /* TODO: make async safe */
    FILE *f = sentry__path_open(path, "rb");
    if (!f) {
        return NULL;
    }
    size_t len = sentry__path_get_size(path);
    if (len == 0) {
        fclose(f);
        char *rv = sentry_malloc(1);
        rv[0] = '\0';
        return rv;
    } else if (len > MAX_READ_TO_BUFFER) {
        fclose(f);
        return NULL;
    }

    /* this is completely not sane in concurrent situations but hey */
    char *rv = sentry_malloc(len + 1);
    if (!rv) {
        fclose(f);
        return NULL;
    }

    size_t read = fread(rv, 1, len, f);
    rv[read] = '\0';
    fclose(f);
    return rv;
}