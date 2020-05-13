#include "sentry_boot.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "sentry_alloc.h"
#include "sentry_backend.h"
#include "sentry_core.h"
#include "sentry_envelope.h"
#include "sentry_modulefinder.h"
#include "sentry_options.h"
#include "sentry_path.h"
#include "sentry_random.h"
#include "sentry_scope.h"
#include "sentry_session.h"
#include "sentry_string.h"
#include "sentry_sync.h"
#include "sentry_transport.h"
#include "sentry_value.h"
#include "transports/sentry_disk_transport.h"

static sentry_options_t *g_options = NULL;
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

    if (sentry__path_create_dir_all(options->database_path)) {
        sentry_options_free(options);
        return 1;
    }
    sentry_path_t *database_path = options->database_path;
    options->database_path = sentry__path_absolute(database_path);
    if (options->database_path) {
        sentry__path_free(database_path);
    } else {
        SENTRY_DEBUG("falling back to non-absolute database path");
        options->database_path = database_path;
    }
    SENTRY_DEBUGF("using database path \"%" SENTRY_PATH_PRI "\"",
        options->database_path->path);

    // try to create and lock our run folder as early as possibly, since it is
    // fallible. since it does locking, it will not interfere with run folder
    // enumeration.
    options->run = sentry__run_new(options->database_path);
    if (!options->run) {
        sentry_options_free(options);
        return 1;
    }

    load_user_consent(options);

    // we "activate" the options here, since the transport and backend might try
    // to access them
    sentry__mutex_lock(&g_options_mutex);
    g_options = options;
    sentry__mutex_unlock(&g_options_mutex);

    sentry_transport_t *transport = options->transport;
    if (transport) {
        sentry__transport_startup(transport, options);
    }

    // after initializing the transport, we will submit all the unsent envelopes
    // and handle remaining sessions.
    sentry__process_old_runs(options);

    // and then we will start the backend, since it requires a valid run
    sentry_backend_t *backend = options->backend;
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
        if (options->transport) {
            // TODO: make this configurable
            // Ideally, it should default to 2s as per
            // https://docs.sentry.io/error-reporting/configuration/?platform=rust#shutdown-timeout
            // but we hit that timeout in our own integration tests, so rather
            // increase it to 5s, as it was before.
            if (!sentry__transport_shutdown(options->transport, 5000)) {
                SENTRY_DEBUG("transport did not shut down cleanly");
            }
        }
        if (options->backend && options->backend->shutdown_func) {
            SENTRY_TRACE("shutting down backend");
            options->backend->shutdown_func(options->backend);
        }
        sentry__run_clean(options->run);
    }

    sentry__mutex_lock(&g_options_mutex);
    sentry_options_free(options);
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
    if (!g_options) {
        sentry__mutex_unlock(&g_options_mutex);
        return;
    }
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
    sentry_user_consent_t rv = SENTRY_USER_CONSENT_UNKNOWN;
    sentry__mutex_lock(&g_options_mutex);
    if (g_options) {
        rv = g_options->user_consent;
    }
    sentry__mutex_unlock(&g_options_mutex);
    return rv;
}

void
sentry__capture_envelope(sentry_envelope_t *envelope)
{
    const sentry_options_t *opts = sentry_get_options();
    bool has_consent = !sentry__should_skip_upload();
    if (opts && opts->transport && has_consent) {
        sentry__transport_send_envelope(opts->transport, envelope);
    } else {
        if (!has_consent) {
            SENTRY_TRACE("discarding envelope due to missing user consent");
        } else {
            SENTRY_TRACE("discarding envelope due to invalid transport");
        }
        sentry_envelope_free(envelope);
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
    if (!opts) {
        return sentry_uuid_nil();
    }
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
sentry_handle_exception(const sentry_ucontext_t *uctx)
{
    const sentry_options_t *opts = sentry_get_options();
    if (!opts) {
        return;
    }
    SENTRY_DEBUG("handling exception");
    if (opts->backend && opts->backend->except_func) {
        opts->backend->except_func(opts->backend, uctx);
    }
}

void
sentry__enforce_disk_transport(void)
{
    // Freeing the old transport would, in the case of the curl transport, try
    // to flush its send queue, which I’m not sure we can do in the signal
    // handler. So rather we just leak it.
    g_options->transport = sentry_new_disk_transport(g_options->run);
}

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

    if (g_options && g_options->backend
        && g_options->backend->add_breadcrumb_func) {
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
