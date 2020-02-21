#include "sentry_scope.h"
#include "sentry_backend.h"
#include "sentry_core.h"
#include "sentry_modulefinder.h"
#include "sentry_sync.h"

static bool g_scope_initialized;
static sentry_scope_t g_scope;
static sentry_mutex_t g_lock = SENTRY__MUTEX_INIT;

static sentry_scope_t *
get_scope(void)
{
    if (g_scope_initialized) {
        return &g_scope;
    }

    g_scope.transaction = NULL;
    g_scope.fingerprint = sentry_value_new_null();
    g_scope.user = sentry_value_new_null();
    g_scope.tags = sentry_value_new_object();
    g_scope.extra = sentry_value_new_object();
    g_scope.contexts = sentry_value_new_object();
    g_scope.breadcrumbs = sentry_value_new_list();
    g_scope.level = SENTRY_LEVEL_ERROR;

    g_scope_initialized = true;

    return &g_scope;
}

void
sentry__scope_cleanup(void)
{
    sentry__mutex_lock(&g_lock);
    if (g_scope_initialized) {
        g_scope_initialized = false;
        sentry_free(g_scope.transaction);
        sentry_value_decref(g_scope.fingerprint);
        sentry_value_decref(g_scope.user);
        sentry_value_decref(g_scope.tags);
        sentry_value_decref(g_scope.extra);
        sentry_value_decref(g_scope.contexts);
        sentry_value_decref(g_scope.breadcrumbs);
    }
    sentry__mutex_unlock(&g_lock);
}

sentry_scope_t *
sentry__scope_lock(void)
{
    sentry__mutex_lock(&g_lock);
    return get_scope();
}

void
sentry__scope_unlock(void)
{
    sentry__mutex_unlock(&g_lock);
}

void
sentry__scope_flush(const sentry_scope_t *scope)
{
    const sentry_options_t *options = sentry_get_options();
    if (options && options->backend && options->backend->flush_scope_func) {
        options->backend->flush_scope_func(options->backend, scope);
    }
}

void
sentry__scope_apply_to_event(
    const sentry_scope_t *scope, sentry_value_t event, sentry_scope_mode_t mode)
{
    const sentry_options_t *options = sentry_get_options();

#define IS_NULL(Key) sentry_value_is_null(sentry_value_get_by_key(event, Key))
#define SET(Key, Value) sentry_value_set_by_key(event, Key, Value)
#define PLACE_STRING(Key, Source)                                              \
    do {                                                                       \
        if (IS_NULL(Key) && Source && *Source) {                               \
            SET(Key, sentry_value_new_string(Source));                         \
        }                                                                      \
    } while (0)
#define PLACE_VALUE(Key, Source)                                               \
    do {                                                                       \
        if (IS_NULL(Key) && !sentry_value_is_null(Source)) {                   \
            sentry_value_incref(Source);                                       \
            SET(Key, Source);                                                  \
        }                                                                      \
    } while (0)

    PLACE_STRING("platform", "native");
    PLACE_STRING("release", options->release);
    PLACE_STRING("dist", options->dist);
    PLACE_STRING("environment", options->environment);

    if (IS_NULL("level")) {
        SET("level", sentry__value_new_level(scope->level));
    }

    PLACE_VALUE("user", scope->user);
    PLACE_VALUE("fingerprint", scope->fingerprint);
    PLACE_STRING("transaction", scope->transaction);

    // TODO: these should merge
    PLACE_VALUE("tags", scope->tags);
    PLACE_VALUE("extra", scope->extra);
    PLACE_VALUE("contexts", scope->contexts);

    if (mode & SENTRY_SCOPE_BREADCRUMBS) {
        PLACE_VALUE("breadcrumbs", sentry__value_clone(scope->breadcrumbs));
    }

    if (mode & SENTRY_SCOPE_MODULES) {
        sentry_value_t modules = sentry__modules_get_list();
        if (!sentry_value_is_null(modules)) {
            sentry_value_t debug_meta = sentry_value_new_object();
            sentry_value_incref(modules);
            sentry_value_set_by_key(debug_meta, "images", modules);
            sentry_value_set_by_key(event, "debug_meta", debug_meta);
        }
    }

#undef PLACE_STRING
#undef IS_NULL
#undef SET
}
