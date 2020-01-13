#include "sentry_boot.h"

#include <stdarg.h>
#include <string.h>

#include "sentry_alloc.h"
#include "sentry_backend.h"
#include "sentry_core.h"
#include "sentry_envelope.h"
#include "sentry_path.h"
#include "sentry_scope.h"
#include "sentry_string.h"
#include "sentry_sync.h"
#include "sentry_value.h"

static sentry_options_t *g_options;
static sentry_mutex_t g_options_mutex = SENTRY__MUTEX_INIT;

static void
load_user_consent(sentry_options_t *opts)
{
    sentry_path_t *consent_path
        = sentry__path_join_str(opts->database_path, "user-consent");
    char *contents = sentry__path_read_to_buffer(consent_path, NULL);
    sentry__path_free(consent_path);
    switch (contents ? contents[0] : 0) {
    case '1':
        opts->user_consent = SENTRY_USER_CONSENT_GIVEN;
        break;
    case '0':
        opts->user_consent = SENTRY_USER_CONSENT_REVOKED;
        break;
    default:
        opts->user_consent = SENTRY_USER_CONSENT_UNKNOWN;
        break;
    }
    sentry_free(contents);
}

bool
sentry__should_skip_upload(void)
{
    sentry__mutex_lock(&g_options_mutex);
    const sentry_options_t *opts = sentry_get_options();
    bool skip = !opts
        || (opts->require_user_consent
            && opts->user_consent != SENTRY_USER_CONSENT_GIVEN);
    sentry__mutex_unlock(&g_options_mutex);
    return skip;
}

int
sentry_init(sentry_options_t *options)
{
    sentry_shutdown();
    sentry__mutex_lock(&g_options_mutex);
    g_options = options;
    sentry__path_create_dir_all(options->database_path);
    load_user_consent(options);
    sentry__mutex_unlock(&g_options_mutex);

    sentry_transport_t *transport = g_options->transport;
    if (transport && transport->startup_func) {
        transport->startup_func(transport);
    }

    sentry_backend_t *backend = g_options->backend;
    if (backend && backend->startup_func) {
        backend->startup_func(backend);
    }

    return 0;
}

void
sentry_shutdown(void)
{
    sentry__mutex_lock(&g_options_mutex);
    if (g_options && g_options->transport
        && g_options->transport->shutdown_func) {
        g_options->transport->shutdown_func(g_options->transport);
    }
    if (g_options && g_options->backend && g_options->backend->shutdown_func) {
        g_options->backend->shutdown_func(g_options->backend);
    }
    sentry_options_free(g_options);
    g_options = NULL;
    sentry__mutex_unlock(&g_options_mutex);
    sentry__scope_cleanup();
}

const sentry_options_t *
sentry_get_options(void)
{
    return g_options;
}

static void
set_user_consent(sentry_user_consent_t new_val)
{
    sentry__mutex_lock(&g_options_mutex);
    g_options->user_consent = new_val;
    sentry__mutex_unlock(&g_options_mutex);
    sentry_path_t *consent_path
        = sentry__path_join_str(g_options->database_path, "user-consent");
    switch (new_val) {
    case SENTRY_USER_CONSENT_GIVEN:
        sentry__path_write_buffer(consent_path, "1\n", 2);
        break;
    case SENTRY_USER_CONSENT_REVOKED:
        sentry__path_write_buffer(consent_path, "0\n", 2);
        break;
    case SENTRY_USER_CONSENT_UNKNOWN:
        sentry__path_remove(consent_path);
        break;
    }
    sentry__path_free(consent_path);

    if (g_options->backend && g_options->backend->user_consent_changed_func) {
        g_options->backend->user_consent_changed_func(g_options->backend);
    }
}

void
sentry_user_consent_give(void)
{
    set_user_consent(SENTRY_USER_CONSENT_GIVEN);
}

void
sentry_user_consent_revoke(void)
{
    set_user_consent(SENTRY_USER_CONSENT_REVOKED);
}

void
sentry_user_consent_reset(void)
{
    set_user_consent(SENTRY_USER_CONSENT_UNKNOWN);
}

sentry_user_consent_t
sentry_user_consent_get(void)
{
    sentry__mutex_lock(&g_options_mutex);
    sentry_user_consent_t rv = g_options->user_consent;
    sentry__mutex_unlock(&g_options_mutex);
    return rv;
}

sentry_uuid_t
sentry_capture_event(sentry_value_t event)
{
    sentry_uuid_t event_id;
    sentry__ensure_event_id(event, &event_id);

    SENTRY_WITH_SCOPE (scope) {
        sentry__scope_apply_to_event(scope, event);
    }

    const sentry_options_t *opts = sentry_get_options();
    if (opts->before_send_func) {
        event = opts->before_send_func(event, NULL, opts->before_send_data);
    }
    if (opts->transport && !sentry_value_is_null(event)) {
        sentry_envelope_t *envelope = sentry__envelope_new();
        if (sentry__envelope_add_event(envelope, event)) {
            opts->transport->send_envelope_func(opts->transport, envelope);
        } else {
            sentry_envelope_free(envelope);
        }
    }

    return event_id;
}

sentry_options_t *
sentry_options_new(void)
{
    sentry_options_t *opts = SENTRY_MAKE(sentry_options_t);
    if (!opts) {
        return NULL;
    }
    memset(opts, 0, sizeof(sentry_options_t));
    opts->database_path = sentry__path_from_str("./.sentry-native");
    opts->user_consent = SENTRY_USER_CONSENT_UNKNOWN;
    opts->backend = sentry__backend_new_default();
    return opts;
}

void
sentry_options_free(sentry_options_t *opts)
{
    if (!opts) {
        return;
    }
    sentry_free(opts->raw_dsn);
    sentry__dsn_cleanup(&opts->dsn);
    sentry_free(opts->release);
    sentry_free(opts->environment);
    sentry_free(opts->dist);
    sentry_free(opts->http_proxy);
    sentry_free(opts->ca_certs);
    sentry__path_free(opts->database_path);
    sentry__path_free(opts->handler_path);
    sentry_transport_free(opts->transport);
    sentry__backend_free(opts->backend);
    sentry_free(opts);
}

void
sentry_transport_free(sentry_transport_t *transport)
{
    if (!transport) {
        return;
    }
    if (transport->free_func) {
        transport->free_func(transport);
    }
    sentry_free(transport);
}

void
sentry_options_set_transport(
    sentry_options_t *opts, sentry_transport_t *transport)
{
    sentry_transport_free(opts->transport);
    opts->transport = transport;
}

void
sentry_options_set_before_send(
    sentry_options_t *opts, sentry_event_function_t func, void *data)
{
    opts->before_send_func = func;
    opts->before_send_data = data;
}

void
sentry_options_set_dsn(sentry_options_t *opts, const char *dsn)
{
    sentry__dsn_cleanup(&opts->dsn);
    /* XXX: log warning here or propagate parsing error */
    sentry_free(opts->raw_dsn);
    sentry__dsn_parse(&opts->dsn, dsn);
    /* TODO: canonicalize DSN */
    opts->raw_dsn = sentry__string_dup(dsn);
}

const char *
sentry_options_get_dsn(const sentry_options_t *opts)
{
    return opts->raw_dsn;
}

void
sentry_options_set_release(sentry_options_t *opts, const char *release)
{
    sentry_free(opts->release);
    opts->release = sentry__string_dup(release);
}

const char *
sentry_options_get_release(const sentry_options_t *opts)
{
    return opts->release;
}

void
sentry_options_set_environment(sentry_options_t *opts, const char *environment)
{
    sentry_free(opts->release);
    opts->release = sentry__string_dup(environment);
}

const char *
sentry_options_get_environment(const sentry_options_t *opts)
{
    return opts->release;
}

void
sentry_options_set_dist(sentry_options_t *opts, const char *dist)
{
    sentry_free(opts->dist);
    opts->dist = sentry__string_dup(dist);
}

const char *
sentry_options_get_dist(const sentry_options_t *opts)
{
    return opts->dist;
}

void
sentry_options_set_http_proxy(sentry_options_t *opts, const char *proxy)
{
    sentry_free(opts->http_proxy);
    opts->http_proxy = sentry__string_dup(proxy);
}

const char *
sentry_options_get_http_proxy(sentry_options_t *opts)
{
    return opts->http_proxy;
}

void
sentry_options_set_ca_certs(sentry_options_t *opts, const char *path)
{
    sentry_free(opts->ca_certs);
    opts->ca_certs = sentry__string_dup(path);
}

const char *
sentry_options_get_ca_certs(const sentry_options_t *opts)
{
    return opts->ca_certs;
}

void
sentry_options_set_debug(sentry_options_t *opts, int debug)
{
    opts->debug = !!debug;
}

int
sentry_options_get_debug(const sentry_options_t *opts)
{
    return opts->debug;
}

void
sentry_options_set_require_user_consent(sentry_options_t *opts, int val)
{
    opts->require_user_consent = true;
}

int
sentry_options_get_require_user_consent(const sentry_options_t *opts)
{
    return opts->require_user_consent;
}

void
sentry_options_set_system_crash_reporter_enabled(
    sentry_options_t *opts, int enabled)
{
    /* TODO: implement */
}

void
sentry_options_set_handler_path(sentry_options_t *opts, const char *path)
{
    sentry__path_free(opts->handler_path);
    opts->handler_path = sentry__path_from_str(path);
}

void
sentry_options_set_database_path(sentry_options_t *opts, const char *path)
{
    sentry__path_free(opts->database_path);
    opts->database_path = sentry__path_from_str(path);
}

#ifdef SENTRY_PLATFORM_WINDOWS
void
sentry_options_set_handler_pathw(sentry_options_t *opts, const wchar_t *path)
{
    sentry__path_free(opts->handler_path);
    opts->handler_path = sentry__path_from_wstr(path);
}

void
sentry_options_set_database_pathw(sentry_options_t *opts, const wchar_t *path)
{
    sentry__path_free(opts->database_path);
    opts->database_path = sentry__path_from_wstr(path);
}
#endif

sentry_uuid_t
sentry__new_event_id(void)
{
#if SENTRY_UNITTEST
    return sentry_uuid_from_string("4c035723-8638-4c3a-923f-2ab9d08b4018");
#else
    return sentry_uuid_new_v4();
#endif
}

sentry_value_t
sentry__ensure_event_id(sentry_value_t event, sentry_uuid_t *uuid_out)
{
    sentry_value_t event_id = sentry_value_get_by_key(event, "event_id");
    const char *uuid_str = sentry_value_as_string(event_id);
    sentry_uuid_t uuid = sentry_uuid_from_string(uuid_str);
    if (sentry_uuid_is_nil(&uuid)) {
        uuid = sentry__new_event_id();
        event_id = sentry__value_new_uuid(&uuid);
        sentry_value_set_by_key(event, "event_id", event_id);
    }
    if (uuid_out) {
        *uuid_out = uuid;
    }
    return event_id;
}
void
sentry_set_user(sentry_value_t user)
{
    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry_value_decref(scope->user);
        scope->user = user;
    }
}

void
sentry_remove_user(void)
{
    sentry_set_user(sentry_value_new_null());
}

void
sentry_add_breadcrumb(sentry_value_t breadcrumb)
{
    sentry_value_incref(breadcrumb);
    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry__value_append_bounded(
            scope->breadcrumbs, breadcrumb, SENTRY_BREADCRUMBS_MAX);
    }

    if (g_options->backend && g_options->backend->add_breadcrumb_func) {
        g_options->backend->add_breadcrumb_func(g_options->backend, breadcrumb);
    } else {
        sentry_value_decref(breadcrumb);
    }
}

void
sentry_set_tag(const char *key, const char *value)
{
    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry_value_set_by_key(
            scope->tags, key, sentry_value_new_string(value));
    }
}

void
sentry_remove_tag(const char *key)
{
    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry_value_remove_by_key(scope->tags, key);
    }
}

void
sentry_set_extra(const char *key, sentry_value_t value)
{
    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry_value_set_by_key(scope->extra, key, value);
    }
}

void
sentry_remove_extra(const char *key)
{
    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry_value_remove_by_key(scope->extra, key);
    }
}

void
sentry_set_context(const char *key, sentry_value_t value)
{
    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry_value_set_by_key(scope->contexts, key, value);
    }
}

void
sentry_remove_context(const char *key)
{
    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry_value_remove_by_key(scope->contexts, key);
    }
}

void
sentry_set_fingerprint(const char *fingerprint, ...)
{
    sentry_value_t fingerprint_value = sentry_value_new_list();

    va_list va;
    va_start(va, fingerprint);
    for (; fingerprint; fingerprint = va_arg(va, const char *)) {
        sentry_value_append(
            fingerprint_value, sentry_value_new_string(fingerprint));
    }
    va_end(va);

    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry_value_decref(scope->fingerprint);
        scope->fingerprint = fingerprint_value;
    };
}

void
sentry_remove_fingerprint(void)
{
    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry_value_decref(scope->fingerprint);
        scope->fingerprint = sentry_value_new_null();
    };
}

void
sentry_set_transaction(const char *transaction)
{
    SENTRY_WITH_SCOPE_MUT (scope) {
        sentry_free(scope->transaction);
        scope->transaction = sentry__string_dup(transaction);
    }
}

void
sentry_remove_transaction(void)
{
    sentry_set_transaction(NULL);
}

void
sentry_set_level(sentry_level_t level)
{
    SENTRY_WITH_SCOPE_MUT (scope) {
        scope->level = level;
    }
}
