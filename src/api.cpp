#include <stdarg.h>
#include <mutex>
#include "attachment.hpp"
#include "backend.hpp"
#include "cleanup.hpp"
#include "internal.hpp"
#include "options.hpp"
#include "scope.hpp"
#include "serialize.hpp"

static sentry_options_t *g_options;
static sentry::Scope g_scope;
static std::mutex scope_lock;
static std::mutex breadcrumb_lock;
static int breadcrumb_fileid = 0;
static int breadcrumb_count = 0;
static sentry::Path event_filename;
static sentry::Path breadcrumb_filename;

#define WITH_LOCKED_BREADCRUMBS \
    std::lock_guard<std::mutex> _blck(breadcrumb_lock)
#define WITH_LOCKED_SCOPE std::lock_guard<std::mutex> _slck(scope_lock)

static void flush_event() {
    mpack_writer_t writer;
    mpack_writer_init_stdfile(&writer, event_filename.open("w"), true);
    sentry::serialize_scope_as_event(&g_scope, &writer);
    mpack_error_t err = mpack_writer_destroy(&writer);
    if (err != mpack_ok) {
        SENTRY_PRINT_ERROR_ARGS("An error occurred encoding the data. Code: %d",
                                err);

        return;
    }
}

int sentry_init(sentry_options_t *options) {
    g_options = options;

    options->runs_folder = options->database_path.join(SENTRY_RUNS_FOLDER);
    sentry::Path current_run_folder =
        options->runs_folder.join(options->run_id.c_str());

    options->attachments.push_back(
        sentry::Attachment(SENTRY_EVENT_FILE_ATTACHMENT_NAME,
                           current_run_folder.join(SENTRY_EVENT_FILE_NAME)));
    options->attachments.push_back(
        sentry::Attachment(SENTRY_BREADCRUMB1_FILE_ATTACHMENT_NAME,
                           current_run_folder.join(SENTRY_BREADCRUMB1_FILE)));
    options->attachments.push_back(
        sentry::Attachment(SENTRY_BREADCRUMB2_FILE_ATTACHMENT_NAME,
                           current_run_folder.join(SENTRY_BREADCRUMB2_FILE)));
    current_run_folder.create_directories();

    event_filename = current_run_folder.join(SENTRY_EVENT_FILE_NAME);

    sentry::init_backend();
    sentry::cleanup_old_runs();

    return 0;
}

const sentry_options_t *sentry_get_options(void) {
    return g_options;
}

void sentry_add_breadcrumb(sentry_breadcrumb_t *breadcrumb) {
    WITH_LOCKED_BREADCRUMBS;

    if (breadcrumb_count == 0 || breadcrumb_count == SENTRY_BREADCRUMBS_MAX) {
        breadcrumb_fileid = breadcrumb_fileid == 0 ? 1 : 0;
        breadcrumb_count = 0;
        breadcrumb_filename =
            g_options->runs_folder.join(g_options->run_id.c_str())
                .join(breadcrumb_fileid == 0 ? SENTRY_BREADCRUMB1_FILE
                                             : SENTRY_BREADCRUMB2_FILE);
    }

    char *data;
    size_t size;
    static mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, &size);
    sentry::serialize_breadcrumb(breadcrumb, &writer);
    if (mpack_writer_destroy(&writer) != mpack_ok) {
        SENTRY_PRINT_ERROR("An error occurred encoding the data.");
        return;
    }

    FILE *file = breadcrumb_filename.open(breadcrumb_count == 0 ? "w" : "a");

    if (file != NULL) {
        fwrite(data, 1, size, file);
        fclose(file);
    }

    free(data);

    breadcrumb_count++;
}

void sentry_set_user(const sentry_user_t *user) {
    WITH_LOCKED_SCOPE;
    g_scope.user.clear();
    if (user->id) {
        g_scope.user.emplace("id", user->id);
    }
    if (user->username) {
        g_scope.user.emplace("username", user->username);
    }
    if (user->email) {
        g_scope.user.emplace("email", user->email);
    }
    if (user->ip_address) {
        g_scope.user.emplace("ip_address", user->ip_address);
    }
    flush_event();
}

void sentry_remove_user() {
    WITH_LOCKED_SCOPE;
    g_scope.user.clear();
    flush_event();
}

void sentry_set_tag(const char *key, const char *value) {
    WITH_LOCKED_SCOPE;
    g_scope.tags.emplace(key, value);
    flush_event();
}

void sentry_remove_tag(const char *key) {
    WITH_LOCKED_SCOPE;
    g_scope.tags.erase(key);
    flush_event();
}

void sentry_set_extra(const char *key, const char *value) {
    WITH_LOCKED_SCOPE;
    g_scope.tags.emplace(key, value);
    flush_event();
}

void sentry_remove_extra(const char *key) {
    WITH_LOCKED_SCOPE;
    g_scope.extra.erase(key);
    flush_event();
}

void sentry_set_fingerprint(const char *fingerprint, ...) {
    WITH_LOCKED_SCOPE;
    va_list va;
    va_start(va, fingerprint);

    if (!fingerprint) {
        g_scope.fingerprint.clear();
    } else {
        g_scope.fingerprint.push_back(fingerprint);
        while (1) {
            const char *arg = va_arg(va, const char *);
            if (!arg) {
                break;
            }
            g_scope.fingerprint.push_back(arg);
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
