#include <assert.h>
#include <stdarg.h>
#include <mutex>
#include "attachment.hpp"
#include "cleanup.hpp"
#include "internal.hpp"
#include "options.hpp"
#include "scope.hpp"
#include "uuid.hpp"
#include "value.hpp"
#include "modulefinders/base.hpp"

using namespace sentry;

static sentry_options_t *g_options;
static Scope g_scope;
static std::mutex scope_lock;

#define WITH_LOCKED_SCOPE std::lock_guard<std::mutex> _slck(scope_lock)

static bool sdk_disabled() {
    return !g_options || g_options->dsn.disabled();
}

static void flush_scope() {
    if (!sdk_disabled() && g_options->backend) {
        g_options->backend->flush_scope_state(g_scope);
    }
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

    if (g_options->transport) {
        g_options->transport->start();
    }

    return 0;
}

void sentry_shutdown(void) {
    if (g_options->transport) {
        g_options->transport->shutdown();
    }
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

    {
        WITH_LOCKED_SCOPE;
        g_scope.apply_to_event(event);
    }

    const sentry_options_t *opts = sentry_get_options();
    if (opts->before_send) {
        event = Value::consume(opts->before_send(event.lower(), nullptr));
    }

	printf("%s\n", event.to_json().c_str());

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

    {
        WITH_LOCKED_SCOPE;
        g_scope.breadcrumbs.append_bounded(breadcrumb_value,
                                           SENTRY_BREADCRUMBS_MAX);
    }

    if (g_options->backend) {
        g_options->backend->add_breadcrumb(breadcrumb_value);
    }
}

void sentry_set_user(sentry_value_t value) {
    WITH_LOCKED_SCOPE;
    g_scope.user = Value::consume(value);
    flush_scope();
}

void sentry_remove_user() {
    WITH_LOCKED_SCOPE;
    g_scope.user = Value();
    flush_scope();
}

void sentry_set_tag(const char *key, const char *value) {
    WITH_LOCKED_SCOPE;
    g_scope.tags.set_by_key(key, Value::new_string(value));
    flush_scope();
}

void sentry_remove_tag(const char *key) {
    WITH_LOCKED_SCOPE;
    g_scope.tags.remove_by_key(key);
    flush_scope();
}

void sentry_set_extra(const char *key, sentry_value_t value) {
    WITH_LOCKED_SCOPE;
    g_scope.extra.set_by_key(key, Value::consume(value));
    flush_scope();
}

void sentry_remove_extra(const char *key) {
    WITH_LOCKED_SCOPE;
    g_scope.extra.remove_by_key(key);
    flush_scope();
}

void sentry_set_fingerprint(const char *fingerprint, ...) {
    WITH_LOCKED_SCOPE;
    va_list va;
    va_start(va, fingerprint);

    g_scope.fingerprint = Value::new_list();
    if (fingerprint) {
        g_scope.fingerprint.append(Value::new_string(fingerprint));
        for (const char *arg; (arg = va_arg(va, const char *));) {
            g_scope.fingerprint.append(Value::new_string(arg));
        }
    }

    va_end(va);

    flush_scope();
}

void sentry_remove_fingerprint(void) {
    sentry_set_fingerprint(nullptr);
}

void sentry_set_transaction(const char *transaction) {
    WITH_LOCKED_SCOPE;
    g_scope.transaction = transaction;
    flush_scope();
}

void sentry_remove_transaction() {
    sentry_set_transaction("");
}

void sentry_set_level(sentry_level_t level) {
    WITH_LOCKED_SCOPE;
    g_scope.level = level;
    flush_scope();
}

void sentry_string_free(char *str) {
    free(str);
}

sentry_value_t sentry_get_module_list() {
    return sentry::modulefinders::get_module_list().lower();
}
