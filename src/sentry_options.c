#include "sentry_options.h"
#include "sentry_alloc.h"
#include "sentry_backend.h"
#include "sentry_database.h"
#include "sentry_path.h"
#include "sentry_string.h"
#include "sentry_transport.h"
#include <stdlib.h>

sentry_options_t *
sentry_options_new(void)
{
    sentry_options_t *opts = SENTRY_MAKE(sentry_options_t);
    if (!opts) {
        return NULL;
    }
    memset(opts, 0, sizeof(sentry_options_t));
    opts->database_path = sentry__path_from_str(".sentry-native");
    // we assume the DSN to be ASCII only
    sentry_options_set_dsn(opts, getenv("SENTRY_DSN"));
    const char *debug = getenv("SENTRY_DEBUG");
    opts->debug = debug && sentry__string_eq(debug, "1");
#ifdef SENTRY_PLATFORM_WINDOWS
    opts->release = sentry__string_from_wstr(_wgetenv(L"SENTRY_RELEASE"));
    opts->environment
        = sentry__string_from_wstr(_wgetenv(L"SENTRY_ENVIRONMENT"));
#else
    opts->release = sentry__string_clone(getenv("SENTRY_RELEASE"));
    opts->environment = sentry__string_clone(getenv("SENTRY_ENVIRONMENT"));
#endif
    opts->user_consent = SENTRY_USER_CONSENT_UNKNOWN;
    opts->system_crash_reporter_enabled = false;
    opts->backend = sentry__backend_new();
    opts->transport = sentry__transport_new_default();
    opts->sample_rate = 1.0;
    return opts;
}

void
sentry__attachment_free(sentry_attachment_t *attachment)
{
    sentry__path_free(attachment->path);
    sentry_free(attachment->name);
    sentry_free(attachment);
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

    sentry_attachment_t *next_attachment = opts->attachments;
    while (next_attachment) {
        sentry_attachment_t *attachment = next_attachment;
        next_attachment = attachment->next;

        sentry__attachment_free(attachment);
    }
    sentry__run_free(opts->run);

    sentry_free(opts);
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
    opts->raw_dsn = sentry__string_clone(dsn);
}

const char *
sentry_options_get_dsn(const sentry_options_t *opts)
{
    return opts->raw_dsn;
}

void
sentry_options_set_sample_rate(sentry_options_t *opts, double sample_rate)
{
    if (sample_rate < 0.0) {
        sample_rate = 0.0;
    } else if (sample_rate > 1.0) {
        sample_rate = 1.0;
    }
    opts->sample_rate = sample_rate;
}

double
sentry_options_get_sample_rate(const sentry_options_t *opts)
{
    return opts->sample_rate;
}

void
sentry_options_set_release(sentry_options_t *opts, const char *release)
{
    sentry_free(opts->release);
    opts->release = sentry__string_clone(release);
}

const char *
sentry_options_get_release(const sentry_options_t *opts)
{
    return opts->release;
}

void
sentry_options_set_environment(sentry_options_t *opts, const char *environment)
{
    sentry_free(opts->environment);
    opts->environment = sentry__string_clone(environment);
}

const char *
sentry_options_get_environment(const sentry_options_t *opts)
{
    return opts->environment;
}

void
sentry_options_set_dist(sentry_options_t *opts, const char *dist)
{
    sentry_free(opts->dist);
    opts->dist = sentry__string_clone(dist);
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
    opts->http_proxy = sentry__string_clone(proxy);
}

const char *
sentry_options_get_http_proxy(const sentry_options_t *opts)
{
    return opts->http_proxy;
}

void
sentry_options_set_ca_certs(sentry_options_t *opts, const char *path)
{
    sentry_free(opts->ca_certs);
    opts->ca_certs = sentry__string_clone(path);
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
    opts->require_user_consent = !!val;
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
    opts->system_crash_reporter_enabled = !!enabled;
}

static void
add_attachment(
    sentry_options_t *opts, const char *orig_name, sentry_path_t *path)
{
    if (!path) {
        return;
    }
    char *name = sentry__string_clone(orig_name);
    if (!name) {
        sentry__path_free(path);
        return;
    }
    sentry_attachment_t *attachment = SENTRY_MAKE(sentry_attachment_t);
    if (!attachment) {
        sentry_free(name);
        sentry__path_free(path);
        return;
    }
    attachment->name = name;
    attachment->path = path;
    attachment->next = opts->attachments;
    opts->attachments = attachment;
}

void
sentry_options_add_attachment(
    sentry_options_t *opts, const char *name, const char *path)
{
    add_attachment(opts, name, sentry__path_from_str(path));
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
sentry_options_add_attachmentw(
    sentry_options_t *opts, const char *name, const wchar_t *path)
{
    add_attachment(opts, name, sentry__path_from_wstr(path));
}

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
