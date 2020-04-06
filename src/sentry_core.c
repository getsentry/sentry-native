#include "sentry_boot.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "sentry_alloc.h"
#include "sentry_backend.h"
#include "sentry_core.h"
#include "sentry_envelope.h"
#include "sentry_modulefinder.h"
#include "sentry_path.h"
#include "sentry_random.h"
#include "sentry_scope.h"
#include "sentry_session.h"
#include "sentry_string.h"
#include "sentry_sync.h"
#include "sentry_transport.h"
#include "sentry_value.h"
#include "transports/sentry_disk_transport.h"

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
        SENTRY_TRACE("starting transport");
        transport->startup_func(transport);
    }

    // after initializing the transport, we will submit all the unsent envelopes
    // and handle remaining sessions.
    sentry__process_old_runs(options);

    // and then create our new run, so it will not interfere with enumerating
    // all the past runs
    options->run = sentry__run_new(options->database_path);

    // and then we will start the backend, since it requires a valid run
    sentry_backend_t *backend = g_options->backend;
    if (backend && backend->startup_func) {
        SENTRY_TRACE("starting backend");
        backend->startup_func(backend);
    }

    return 0;
}

void
sentry_shutdown(void)
{
    sentry_end_session();

    sentry__mutex_lock(&g_options_mutex);
    sentry_options_t *options = g_options;
    sentry__mutex_unlock(&g_options_mutex);

    if (options) {
        if (options->transport && options->transport->shutdown_func) {
            SENTRY_TRACE("shutting down transport");
            options->transport->shutdown_func(options->transport);
        }
        if (options->backend && options->backend->shutdown_func) {
            SENTRY_TRACE("shutting down backend");
            options->backend->shutdown_func(options->backend);
        }
        sentry__run_clean(options->run);
    }

    sentry__mutex_lock(&g_options_mutex);
    sentry_options_free(g_options);
    g_options = NULL;
    sentry__mutex_unlock(&g_options_mutex);
    sentry__scope_cleanup();
    sentry__modulefinder_cleanup();
}

void
sentry_clear_modulecache(void)
{
    sentry__modulefinder_cleanup();
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

void
sentry__capture_envelope(sentry_envelope_t *envelope)
{
    const sentry_options_t *opts = sentry_get_options();
    if (opts->transport) {
        SENTRY_TRACE("sending envelope");
        opts->transport->send_envelope_func(opts->transport, envelope);
    }
}

static bool
event_is_considered_error(sentry_value_t event)
{
    const char *level
        = sentry_value_as_string(sentry_value_get_by_key(event, "level"));
    if (sentry__string_eq(level, "fatal")
        || sentry__string_eq(level, "error")) {
        return true;
    }
    if (!sentry_value_is_null(sentry_value_get_by_key(event, "exception"))) {
        return true;
    }
    return false;
}

sentry_uuid_t
sentry_capture_event(sentry_value_t event)
{
    const sentry_options_t *opts = sentry_get_options();
    uint64_t rnd;
    if (opts->sample_rate < 1.0 && !sentry__getrandom(&rnd, sizeof(rnd))
        && ((double)rnd / (double)UINT64_MAX) > opts->sample_rate) {
        SENTRY_DEBUG("throwing away event due to sample rate");
        sentry_value_decref(event);
        return sentry_uuid_nil();
    }

    SENTRY_DEBUG("capturing event");
    sentry_uuid_t event_id;
    sentry__ensure_event_id(event, &event_id);

    SENTRY_WITH_SCOPE (scope) {
        SENTRY_TRACE("merging scope into event");
        sentry__scope_apply_to_event(scope, event, SENTRY_SCOPE_ALL);
    }

    if (opts->before_send_func) {
        event = opts->before_send_func(event, NULL, opts->before_send_data);
    }
    if (opts->transport && !sentry_value_is_null(event)) {
        sentry_envelope_t *envelope = sentry__envelope_new();
        if (!envelope) {
            return event_id;
        }

        SENTRY_TRACE("adding attachments to envelope");
        for (sentry_attachment_t *attachment = opts->attachments; attachment;
             attachment = attachment->next) {
            sentry_envelope_item_t *item = sentry__envelope_add_from_path(
                envelope, attachment->path, "attachment");
            if (!item) {
                continue;
            }
            sentry__envelope_item_set_header(
                item, "name", sentry_value_new_string(attachment->name));
            sentry__envelope_item_set_header(item, "filename",
#ifdef SENTRY_PLATFORM_WINDOWS
                sentry__value_new_string_from_wstr(
#else
                sentry_value_new_string(
#endif
                    sentry__path_filename(attachment->path)));
        }

        if (event_is_considered_error(sentry_envelope_get_event(envelope))) {
            sentry__record_errors_on_current_session(1);
        }
        sentry__add_current_session_to_envelope(envelope);

        if (sentry__envelope_add_event(envelope, event)) {
            sentry__capture_envelope(envelope);
        } else {
            sentry_envelope_free(envelope);
        }
    }

    return event_id;
}

void
sentry_handle_exception(sentry_ucontext_t *uctx)
{
    SENTRY_DEBUG("handling exception");
    if (g_options->backend && g_options->backend->except_func) {
        g_options->backend->except_func(g_options->backend, uctx);
    }
}

sentry_options_t *
sentry_options_new(void)
{
    sentry_options_t *opts = SENTRY_MAKE(sentry_options_t);
    if (!opts) {
        return NULL;
    }
    memset(opts, 0, sizeof(sentry_options_t));
    opts->database_path = sentry__path_from_str(".sentry-native");
    sentry_options_set_dsn(opts, getenv("SENTRY_DSN"));
    opts->release = sentry__string_clone(getenv("SENTRY_RELEASE"));
    opts->environment = sentry__string_clone(getenv("SENTRY_ENVIRONMENT"));
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
sentry__enforce_disk_transport(void)
{
    // Freeing the old transport would, in the case of the curl transport, try
    // to flush its send queue, which I’m not sure we can do in the signal
    // handler. So rather we just leak it.
    g_options->transport = sentry_new_disk_transport(g_options->run);
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
        sentry__scope_session_sync(scope);
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
    // the `no_flush` will avoid triggering *both* scope-change and
    // breadcrumb-add events.
    SENTRY_WITH_SCOPE_MUT_NO_FLUSH(scope)
    {
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
        scope->transaction = sentry__string_clone(transaction);
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
