#include <assert.h>
#include <stdarg.h>
#include <mutex>
#include "attachment.hpp"
#include "backend.hpp"
#include "cleanup.hpp"
#include "internal.hpp"
#include "options.hpp"
#include "scope.hpp"
#include "uuid.hpp"
#include "value.hpp"

using namespace sentry;

static sentry_options_t *g_options;
static Scope g_scope;
static std::mutex scope_lock;
static std::mutex breadcrumb_lock;
static int breadcrumb_fileid = 0;
static int breadcrumbs_in_segment = 0;
static Path event_filename;
static Path breadcrumb_filename;

#define WITH_LOCKED_BREADCRUMBS \
    std::lock_guard<std::mutex> _blck(breadcrumb_lock)
#define WITH_LOCKED_SCOPE std::lock_guard<std::mutex> _slck(scope_lock)

static bool sdk_disabled() {
    return !g_options || g_options->dsn.disabled();
}

static void flush_event() {
    if (sdk_disabled()) {
        return;
    }

    mpack_writer_t writer;
    mpack_writer_init_stdfile(&writer, event_filename.open("w"), true);
    Value event = Value::newEvent();
    event.toMsgpack(&writer);
    mpack_error_t err = mpack_writer_destroy(&writer);
    if (err != mpack_ok) {
        SENTRY_LOGF("An error occurred encoding the data. Code: %d", err);
        return;
    }
}

int sentry_init(sentry_options_t *options) {
    assert(!g_options);
    g_options = options;

    options->runs_folder = options->database_path.join(SENTRY_RUNS_FOLDER);
    Path current_run_folder =
        options->runs_folder.join(options->run_id.c_str());

    options->attachments.emplace_back(
        Attachment(SENTRY_EVENT_FILE_ATTACHMENT_NAME,
                   current_run_folder.join(SENTRY_EVENT_FILE_NAME)));
    options->attachments.emplace_back(
        Attachment(SENTRY_BREADCRUMB1_FILE_ATTACHMENT_NAME,
                   current_run_folder.join(SENTRY_BREADCRUMB1_FILE)));
    options->attachments.emplace_back(
        Attachment(SENTRY_BREADCRUMB2_FILE_ATTACHMENT_NAME,
                   current_run_folder.join(SENTRY_BREADCRUMB2_FILE)));

    if (!options->dsn.disabled()) {
        SENTRY_LOGF("crash handler enabled (reporting to %s)",
                    options->dsn.raw());
        current_run_folder.create_directories();
        event_filename = current_run_folder.join(SENTRY_EVENT_FILE_NAME);
        init_backend();
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
    Value event_id = event.getByKey("event_id");

    if (event_id.isNull()) {
        uuid = sentry_uuid_new_v4();
        char uuid_str[40];
        sentry_uuid_as_string(&uuid, uuid_str);
        event.setKey("event_id", Value::newString(uuid_str));
    } else {
        uuid = sentry_uuid_from_string(event_id.asCStr());
    }

    {
        WITH_LOCKED_SCOPE;
        g_scope.applyToEvent(event);
    }

    const sentry_options_t *opts = sentry_get_options();
    if (opts->before_send) {
        event = Value::consume(opts->before_send(event.lower(), nullptr));
    }

    if (opts->transport && !event.isNull()) {
        opts->transport->sendEvent(event);
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
        g_scope.breadcrumbs.appendBounded(breadcrumb_value,
                                          SENTRY_BREADCRUMBS_MAX);
    }

    WITH_LOCKED_BREADCRUMBS;
    if (breadcrumbs_in_segment == 0 ||
        breadcrumbs_in_segment == SENTRY_BREADCRUMBS_MAX) {
        breadcrumb_fileid = breadcrumb_fileid == 0 ? 1 : 0;
        breadcrumbs_in_segment = 0;
        breadcrumb_filename =
            g_options->runs_folder.join(g_options->run_id.c_str())
                .join(breadcrumb_fileid == 0 ? SENTRY_BREADCRUMB1_FILE
                                             : SENTRY_BREADCRUMB2_FILE);
    }

    std::string mpack = breadcrumb_value.toMsgpack();
    FILE *file =
        breadcrumb_filename.open(breadcrumbs_in_segment == 0 ? "w" : "a");
    if (file) {
        fwrite(mpack.c_str(), 1, mpack.size(), file);
        fclose(file);
    }

    breadcrumbs_in_segment++;
}

void sentry_set_user(sentry_value_t value) {
    WITH_LOCKED_SCOPE;
    g_scope.user = Value::consume(value);
    flush_event();
}

void sentry_remove_user() {
    WITH_LOCKED_SCOPE;
    g_scope.user = Value();
    flush_event();
}

void sentry_set_tag(const char *key, const char *value) {
    WITH_LOCKED_SCOPE;
    g_scope.tags.setKey(key, Value::newString(value));
    flush_event();
}

void sentry_remove_tag(const char *key) {
    WITH_LOCKED_SCOPE;
    g_scope.tags.removeKey(key);
    flush_event();
}

void sentry_set_extra(const char *key, sentry_value_t value) {
    WITH_LOCKED_SCOPE;
    g_scope.extra.setKey(key, Value::consume(value));
    flush_event();
}

void sentry_remove_extra(const char *key) {
    WITH_LOCKED_SCOPE;
    g_scope.extra.removeKey(key);
    flush_event();
}

void sentry_set_fingerprint(const char *fingerprint, ...) {
    WITH_LOCKED_SCOPE;
    va_list va;
    va_start(va, fingerprint);

    g_scope.fingerprint = Value::newList();
    if (fingerprint) {
        g_scope.fingerprint.append(Value::newString(fingerprint));
        for (const char *arg; (arg = va_arg(va, const char *));) {
            g_scope.fingerprint.append(Value::newString(arg));
        }
    }

    va_end(va);

    flush_event();
}

void sentry_remove_fingerprint(void) {
    sentry_set_fingerprint(NULL);
}

void sentry_set_transaction(const char *transaction) {
    WITH_LOCKED_SCOPE;
    g_scope.transaction = transaction;
    flush_event();
}

void sentry_remove_transaction() {
    sentry_set_transaction("");
}

void sentry_set_level(sentry_level_t level) {
    WITH_LOCKED_SCOPE;
    g_scope.level = level;
    flush_event();
}
