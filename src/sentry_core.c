#include "sentry_core.h"
#include "sentry_alloc.h"
#include "sentry_path.h"
#include "sentry_string.h"
#include "sentry_sync.h"
#include <string.h>

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

    return 0;
}

void
sentry_shutdown(void)
{
    sentry__mutex_lock(&g_options_mutex);
    sentry_options_free(g_options);
    g_options = NULL;
    sentry__mutex_unlock(&g_options_mutex);
}

const sentry_options_t *
sentry_get_options(void)
{
    return g_options;
}

void
sentry_user_consent_give(void)
{
    sentry__mutex_lock(&g_options_mutex);
    g_options->user_consent = SENTRY_USER_CONSENT_GIVEN;
    sentry__mutex_unlock(&g_options_mutex);
    sentry_path_t *consent_path
        = sentry__path_join_str(g_options->database_path, "user-consent");
    sentry__path_write_buffer(consent_path, "1\n", 2);
    sentry__path_free(consent_path);
}

void
sentry_user_consent_revoke(void)
{
    sentry__mutex_lock(&g_options_mutex);
    g_options->user_consent = SENTRY_USER_CONSENT_REVOKED;
    sentry__mutex_unlock(&g_options_mutex);
    sentry_path_t *consent_path
        = sentry__path_join_str(g_options->database_path, "user-consent");
    sentry__path_write_buffer(consent_path, "0\n", 2);
    sentry__path_free(consent_path);
}

void
sentry_user_consent_reset(void)
{
    sentry__mutex_lock(&g_options_mutex);
    g_options->user_consent = SENTRY_USER_CONSENT_UNKNOWN;
    sentry__mutex_unlock(&g_options_mutex);
    sentry_path_t *consent_path
        = sentry__path_join_str(g_options->database_path, "user-consent");
    sentry__path_remove(consent_path);
    sentry__path_free(consent_path);
}

sentry_user_consent_t
sentry_user_consent_get(void)
{
    sentry__mutex_lock(&g_options_mutex);
    sentry_user_consent_t rv = g_options->user_consent;
    sentry__mutex_unlock(&g_options_mutex);
    return rv;
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
    sentry_transport_free(opts->transport);
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

#if SENTRY_PLATFORM == SENTRY_PLATFORM_WINDOWS
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