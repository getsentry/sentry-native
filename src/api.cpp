#include <assert.h>
#include <stdarg.h>
#include <fstream>
#include <mutex>

#include "attachment.hpp"
#include "cleanup.hpp"
#include "internal.hpp"
#include "io.hpp"
#include "modulefinder.hpp"
#include "options.hpp"
#include "scope.hpp"
#include "transports/base_transport.hpp"
#include "unwind.hpp"
#include "uuid.hpp"
#include "value.hpp"

using namespace sentry;

static sentry_options_t *g_options;

static bool sdk_disabled() {
    return !g_options || g_options->dsn.disabled();
}

int sentry_init(sentry_options_t *options) {
    assert(!g_options);
    g_options = options;

    options->runs_folder = options->database_path.join(SENTRY_RUNS_FOLDER);
    if (!options->backend) {
        SENTRY_LOG("crash handler disabled because no backend configured");
    } else if (!options->dsn.disabled()) {
        SENTRY_LOGF("crash handler enabled (reporting to %s)",
                    options->dsn.raw());
        g_options->backend->start();
    } else {
        SENTRY_LOG("crash handler disabled because DSN is empty");
    }
    cleanup_old_runs();

    // Check for stored user consent
    sentry::Path consent_path = options->database_path.join("user-consent");
    sentry::FileIoReader r;
    if (r.open(consent_path)) {
        char c = r.read_char();
        if (c == '1') {
            options->user_consent = SENTRY_USER_CONSENT_GIVEN;
        } else if (c == '0') {
            options->user_consent = SENTRY_USER_CONSENT_REVOKED;
        }
    }

    // make sure that the scopes are at least flushed once after the backend
    // is started for the initial data to be written.
    Scope::with_scope_mut([](Scope &) { /* run for side effect */ });

    if (g_options->transport) {
        g_options->transport->start();
    }

    return 0;
}

void sentry_shutdown(void) {
    if (g_options) {
        if (g_options->transport) {
            g_options->transport->shutdown();
        }
        sentry_options_free(g_options);
    }
    g_options = nullptr;
}

void sentry_user_consent_give(void) {
    g_options->database_path.create_directories();
    sentry::Path consent_path = g_options->database_path.join("user-consent");
    sentry::FileIoWriter w;
    if (w.open(consent_path)) {
        w.write_char('1');
        w.write_char('\n');
        g_options->user_consent = SENTRY_USER_CONSENT_GIVEN;
        if (g_options->backend) {
            g_options->backend->user_consent_changed();
        }
    }
}

void sentry_user_consent_revoke(void) {
    g_options->database_path.create_directories();
    sentry::Path consent_path = g_options->database_path.join("user-consent");
    sentry::FileIoWriter w;
    if (w.open(consent_path)) {
        w.write_char('0');
        w.write_char('\n');
        g_options->user_consent = SENTRY_USER_CONSENT_REVOKED;
        if (g_options->backend) {
            g_options->backend->user_consent_changed();
        }
    }
}

void sentry_user_consent_reset(void) {
    sentry::Path consent_path = g_options->database_path.join("user-consent");
    consent_path.remove();
    g_options->user_consent = SENTRY_USER_CONSENT_UNKNOWN;
    if (g_options->backend) {
        g_options->backend->user_consent_changed();
    }
}

sentry_user_consent_t sentry_user_consent_get(void) {
    return g_options->user_consent;
}

const sentry_options_t *sentry_get_options(void) {
    return g_options;
}

sentry_uuid_t sentry_capture_event(sentry_value_t evt) {
    Value event = Value::consume(evt);
    sentry_uuid_t uuid;
    Value event_id = event.get_by_key("event_id");

    if (event_id.is_null()) {
        uuid = sentry_uuid_new_v4();
        char uuid_str[40];
        sentry_uuid_as_string(&uuid, uuid_str);
        event.set_by_key("event_id", Value::new_string(uuid_str));
    } else {
        uuid = sentry_uuid_from_string(event_id.as_cstr());
    }

    Scope::with_scope(
        [&event](const Scope &scope) { scope.apply_to_event(event); });

    const sentry_options_t *opts = sentry_get_options();
    if (opts->before_send) {
        event = opts->before_send(event, nullptr);
    }

    if (opts->transport && !event.is_null()) {
        opts->transport->send_event(event);
    }

    return uuid;
}

void sentry_add_breadcrumb(sentry_value_t breadcrumb) {
    Value breadcrumb_value = Value::consume(breadcrumb);
    if (sdk_disabled()) {
        return;
    }

    Scope::with_scope_mut([breadcrumb_value](Scope &scope) {
        scope.breadcrumbs.append_bounded(breadcrumb_value,
                                         SENTRY_BREADCRUMBS_MAX);
    });

    if (g_options->backend) {
        g_options->backend->add_breadcrumb(breadcrumb_value);
    }
}

void sentry_set_user(sentry_value_t value) {
    Scope::with_scope_mut(
        [value](Scope &scope) { scope.user = Value::consume(value); });
}

void sentry_remove_user() {
    Scope::with_scope_mut([](Scope &scope) { scope.user = Value(); });
}

void sentry_set_tag(const char *key, const char *value) {
    Scope::with_scope_mut([key, value](Scope &scope) {
        scope.tags.set_by_key(key, Value::new_string(value));
    });
}

void sentry_remove_tag(const char *key) {
    Scope::with_scope_mut(
        [key](Scope &scope) { scope.tags.remove_by_key(key); });
}

void sentry_set_extra(const char *key, sentry_value_t value) {
    Scope::with_scope_mut([key, value](Scope &scope) {
        scope.extra.set_by_key(key, Value::consume(value));
    });
}

void sentry_remove_extra(const char *key) {
    Scope::with_scope_mut(
        [key](Scope &scope) { scope.extra.remove_by_key(key); });
}

void sentry_set_context(const char *key, sentry_value_t value) {
    Scope::with_scope_mut([key, value](Scope &scope) {
        scope.contexts.set_by_key(key, Value::consume(value));
    });
}

void sentry_remove_context(const char *key) {
    Scope::with_scope_mut(
        [key](Scope &scope) { scope.contexts.remove_by_key(key); });
}

void sentry_set_fingerprint(const char *fingerprint, ...) {
    Value fingerprint_value = Value::new_list();

    va_list va;
    va_start(va, fingerprint);
    for (; fingerprint; fingerprint = va_arg(va, const char *)) {
        fingerprint_value.append(Value::new_string(fingerprint));
    }
    va_end(va);

    Scope::with_scope_mut([fingerprint_value](Scope &scope) {
        scope.fingerprint = fingerprint_value;
    });
}

void sentry_remove_fingerprint(void) {
    sentry_set_fingerprint(nullptr);
}

void sentry_set_transaction(const char *transaction) {
    Scope::with_scope_mut(
        [transaction](Scope &scope) { scope.transaction = transaction; });
}

void sentry_remove_transaction() {
    sentry_set_transaction("");
}

void sentry_set_level(sentry_level_t level) {
    Scope::with_scope_mut([level](Scope &scope) { scope.level = level; });
}

void sentry_string_free(char *str) {
    free(str);
}

size_t sentry_unwind_stack(void *addr, void **stacktrace_out, size_t max_len) {
    return unwind_stack(addr, nullptr, stacktrace_out, max_len);
}

size_t sentry_unwind_stack_from_ucontext(const sentry_ucontext_t *uctx,
                                         void **stacktrace_out,
                                         size_t max_len) {
    return unwind_stack(nullptr, uctx, stacktrace_out, max_len);
}

sentry_value_t sentry_envelope_get_event(const sentry_envelope_t *envelope) {
    const transports::Envelope *e = (const transports::Envelope *)envelope;
    return e->get_event().lower_decref();
}

char *sentry_envelope_serialize(const sentry_envelope_t *envelope,
                                size_t *size_out) {
    const transports::Envelope *e = (const transports::Envelope *)envelope;

    std::string data = e->serialize(size_out);
    char* pdata = (char*)calloc(data.size() + 1, sizeof(char));
    if(pdata) {
        memcpy(pdata, data.c_str(), data.size());
    }
    return pdata;
}

int sentry_envelope_write_to_file(const sentry_envelope_t *envelope,
                                  const char *path) {
    const transports::Envelope *e = (const transports::Envelope *)envelope;
    FileIoWriter writer;
    if (!writer.open(path)) {
        return 1;
    }
    e->serialize_into(writer);
    return 0;
}
