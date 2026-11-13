#include "sentry_backend.h"
#include "sentry_string.h"

#include <stdlib.h>

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

sentry_backend_t *sentry__backend_crashpad_new(void);
sentry_backend_t *sentry__backend_breakpad_new(void);
sentry_backend_t *sentry__backend_inproc_new(void);
void sentry__backend_inproc_preload(void);
sentry_backend_t *sentry__backend_native_new(void);
void sentry__backend_native_preload(void);

typedef struct {
    const char *name;
    sentry_backend_t *(*new_func)(void);
    void (*preload_func)(void);
} backend_entry_t;

static const backend_entry_t g_backends[] = {
#ifdef SENTRY_BACKEND_CRASHPAD
    { "crashpad", sentry__backend_crashpad_new, NULL },
#endif
#ifdef SENTRY_BACKEND_BREAKPAD
    { "breakpad", sentry__backend_breakpad_new, NULL },
#endif
#ifdef SENTRY_BACKEND_INPROC
    { "inproc", sentry__backend_inproc_new, sentry__backend_inproc_preload },
#endif
#ifdef SENTRY_BACKEND_NATIVE
    { "native", sentry__backend_native_new, sentry__backend_native_preload },
#endif
    { "none", NULL, NULL },
};

static const char *g_backend = NULL;

static const backend_entry_t *
find_backend(const char *name)
{
    const size_t count = sizeof(g_backends) / sizeof(g_backends[0]);
    for (size_t i = 0; name && i < count; i++) {
        if (sentry__string_eq(g_backends[i].name, name)) {
            return &g_backends[i];
        }
    }
    return NULL;
}

int
sentry_backend_preload(const char *name)
{
    const backend_entry_t *entry = find_backend(name);
    if (!entry) {
        return 1;
    }
    g_backend = entry->name;
    if (entry->preload_func) {
        entry->preload_func();
    }
    return 0;
}

#ifndef SENTRY_BACKEND_CUSTOM
static const backend_entry_t *
resolve_backend(void)
{
    // SENTRY_BACKEND overrides the default for testing
    const backend_entry_t *entry
        = find_backend(g_backend ? g_backend : getenv("SENTRY_BACKEND"));
    return entry ? entry : find_backend(SENTRY_BACKEND_DEFAULT);
}

void
sentry__backend_preload(void)
{
    const backend_entry_t *entry = resolve_backend();
    if (entry && entry->preload_func) {
        entry->preload_func();
    }
}

sentry_backend_t *
sentry__backend_new(void)
{
    const backend_entry_t *entry = resolve_backend();
    if (!entry || !entry->new_func) {
        return NULL;
    }
    sentry_backend_t *backend = entry->new_func();
    if (backend) {
        backend->name = entry->name;
    }
    return backend;
}
#endif
