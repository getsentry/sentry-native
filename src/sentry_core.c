#include "sentry_core.h"
#include "sentry_alloc.h"
#include "sentry_string.h"
#include <string.h>

static sentry_options_t *g_options;

int
sentry_init(sentry_options_t *options)
{
    g_options = options;
    return 0;
}

void
sentry_shutdown(void)
{
}

const sentry_options_t *
sentry_get_options(void)
{
    return g_options;
}

void
sentry_user_consent_give(void)
{
    g_options->user_consent = SENTRY_USER_CONSENT_GIVEN;
}

void
sentry_user_consent_revoke(void)
{
    g_options->user_consent = SENTRY_USER_CONSENT_REVOKED;
}

void
sentry_user_consent_reset(void)
{
    g_options->user_consent = SENTRY_USER_CONSENT_UNKNOWN;
}

sentry_user_consent_t
sentry_user_consent_get(void)
{
    return g_options->user_consent;
}

sentry_options_t *
sentry_options_new(void)
{
    sentry_options_t *opts = SENTRY_MAKE(sentry_options_t);
    if (!opts) {
        return NULL;
    }
    memset(opts, 0, sizeof(sentry_options_t));
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
    sentry_free(opts);
}

void
sentry_options_set_transport(
    sentry_options_t *opts, sentry_transport_function_t func, void *data)
{
    opts->transport_func = func;
    opts->transport_data = data;
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