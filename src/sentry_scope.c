#include "sentry_scope.h"
#include "sentry_backend.h"
#include "sentry_core.h"
#include "sentry_database.h"
#include "sentry_modulefinder.h"
#include "sentry_options.h"
#include "sentry_string.h"
#include "sentry_sync.h"

static bool g_scope_initialized;
static sentry_scope_t g_scope;
static sentry_mutex_t g_lock = SENTRY__MUTEX_INIT;

static sentry_value_t
get_client_sdk(void)
{
    sentry_value_t client_sdk = sentry_value_new_object();

    sentry_value_t name = sentry_value_new_string(SENTRY_SDK_NAME);
    sentry_value_set_by_key(client_sdk, "name", name);

    sentry_value_t version = sentry_value_new_string(SENTRY_SDK_VERSION);
    sentry_value_set_by_key(client_sdk, "version", version);

    sentry_value_t package = sentry_value_new_object();

    sentry_value_t package_name
        = sentry_value_new_string("github:getsentry/sentry-native");
    sentry_value_set_by_key(package, "name", package_name);

    sentry_value_incref(version);
    sentry_value_set_by_key(package, "version", version);

    sentry_value_t packages = sentry_value_new_list();
    sentry_value_append(packages, package);
    sentry_value_set_by_key(client_sdk, "packages", packages);

    sentry_value_freeze(client_sdk);
    return client_sdk;
}

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
    g_scope.client_sdk = get_client_sdk();
    g_scope.session = NULL;

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
        sentry_value_decref(g_scope.client_sdk);
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
    if (!options) {
        return;
    }
    if (options->backend && options->backend->flush_scope_func) {
        options->backend->flush_scope_func(options->backend, scope);
    }
    if (scope->session) {
        sentry__run_write_session(options->run, scope->session);
    } else {
        sentry__run_clear_session(options->run);
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
    PLACE_VALUE("sdk", scope->client_sdk);

    // TODO: these should merge
    PLACE_VALUE("tags", scope->tags);
    PLACE_VALUE("extra", scope->extra);
    PLACE_VALUE("contexts", scope->contexts);

    if (mode & SENTRY_SCOPE_BREADCRUMBS) {
        sentry_value_t breadcrumbs = sentry__value_clone(scope->breadcrumbs);
        PLACE_VALUE("breadcrumbs", breadcrumbs);
        // because `PLACE_VALUE` adds a new ref, and we would otherwise leak
        sentry_value_decref(breadcrumbs);
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

void
sentry__scope_session_sync(sentry_scope_t *scope)
{
    if (!scope->session) {
        return;
    }

    if (!sentry_value_is_null(scope->user)) {
        sentry_value_t did = sentry_value_get_by_key(scope->user, "id");
        if (sentry_value_is_null(did)) {
            did = sentry_value_get_by_key(scope->user, "email");
        }
        if (sentry_value_is_null(did)) {
            did = sentry_value_get_by_key(scope->user, "username");
        }
        sentry_value_decref(scope->session->distinct_id);
        sentry_value_incref(did);
        scope->session->distinct_id = did;
    }
}
