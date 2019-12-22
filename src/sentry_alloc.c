#include <stdlib.h>
#include <string.h>

void *
sentry_malloc(size_t size)
{
    return malloc(size);
}

void *
sentry_realloc(char *ptr, size_t size)
{
    return realloc(ptr, size);
}

void
sentry_free(void *ptr)
{
    free(ptr);
}