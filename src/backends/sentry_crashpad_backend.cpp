#include "sentry_crashpad_backend.h"

extern "C" {
#include "../sentry_alloc.h"
#include "../sentry_core.h"
#include "../sentry_database.h"
#include "../sentry_path.h"
#include "../sentry_utils.h"
}

#include "client/crash_report_database.h"
#include "client/crashpad_client.h"
#include "client/crashpad_info.h"
#include "client/settings.h"
#include <map>
#include <vector>

typedef struct {
    sentry_path_t *event_path;
    sentry_path_t *breadcrumb1_path;
    sentry_path_t *breadcrumb2_path;
    size_t num_breadcrumbs;
} crashpad_state_t;

static std::unique_ptr<crashpad::CrashReportDatabase> g_db;

static void
user_consent_changed(sentry_backend_t *backend)
{
    if (!g_db || !g_db->GetSettings()) {
        return;
    }
    g_db->GetSettings()->SetUploadsEnabled(!sentry__should_skip_upload());
}

static void
startup_crashpad_backend(sentry_backend_t *backend)
{
    // TODO: backends should really get the options as argument
    const sentry_options_t *options = sentry_get_options();
    if (!options->handler_path
        || !sentry__path_is_file(options->handler_path)) {
        SENTRY_DEBUG("unable to start crashpad backend, invalid handler_path");
        return;
    }

    SENTRY_TRACE("starting crashpad backend");
    sentry_path_t *current_run_folder = options->run->run_path;
    crashpad_state_t *data = (crashpad_state_t *)backend->data;

    base::FilePath database(options->database_path->path);
    base::FilePath handler(options->handler_path->path);

    std::map<std::string, std::string> annotations;
    std::map<std::string, base::FilePath> file_attachments;

    // register attachments
    for (sentry_attachment_t *attachment = options->attachments; attachment;
         attachment = attachment->next) {
        file_attachments.emplace(
            attachment->name, base::FilePath(attachment->path->path));
    }

    // and add the serialized event, and two rotating breadcrumb files
    // as attachments and make sure the files exist
    data->event_path
        = sentry__path_join_str(current_run_folder, "__sentry-event");
    data->breadcrumb1_path
        = sentry__path_join_str(current_run_folder, "__sentry-breadcrumb1");
    data->breadcrumb2_path
        = sentry__path_join_str(current_run_folder, "__sentry-breadcrumb2");

    sentry__path_touch(data->event_path);
    sentry__path_touch(data->breadcrumb1_path);
    sentry__path_touch(data->breadcrumb2_path);

    file_attachments.emplace(
        "__sentry-event", base::FilePath(data->event_path->path));
    file_attachments.emplace(
        "__sentry-breadcrumb1", base::FilePath(data->breadcrumb1_path->path));
    file_attachments.emplace(
        "__sentry-breadcrumb2", base::FilePath(data->breadcrumb2_path->path));

    std::vector<std::string> arguments;
    arguments.push_back("--no-rate-limit");

#ifdef _WIN32
    // Temporary fix for Windows
    arguments.push_back("--no-upload-gzip");
#endif

    // initialize database first and check for user consent.  This is going
    // to change the setting persisted into the crashpad database.  The
    // update to the consent change is then reflected when the handler starts.
    g_db = crashpad::CrashReportDatabase::Initialize(database);
    user_consent_changed(backend);

    crashpad::CrashpadClient client;
    std::string url(sentry__dsn_get_minidump_url(&options->dsn));
    bool success = client.StartHandlerWithAttachments(handler, database,
        database, url, annotations, file_attachments, arguments,
        /* restartable */ true,
        /* asynchronous_start */ false);

    if (success) {
        SENTRY_DEBUG("started crashpad client handler.");
    } else {
        SENTRY_DEBUG("failed to start crashpad client handler.");
        return;
    }

    if (!options->system_crash_reporter_enabled) {
        // Disable the system crash reporter. Especially on macOS, it takes
        // substantial time *after* crashpad has done its job.
        crashpad::CrashpadInfo *crashpad_info
            = crashpad::CrashpadInfo::GetCrashpadInfo();
        crashpad_info->set_system_crash_reporter_forwarding(
            crashpad::TriState::kDisabled);
    }
}

static void
flush_scope(sentry_backend_t *backend, const sentry_scope_t *scope)
{
    sentry_value_t event = sentry_value_new_object();
    SENTRY_WITH_SCOPE (scope) {
        sentry__scope_apply_to_event(scope, event);
    }

    size_t mpack_size;
    char *mpack = sentry_value_to_msgpack(event, &mpack_size);
    sentry_value_decref(event);

    crashpad_state_t *data = (crashpad_state_t *)backend->data;
    int _rv = sentry__path_write_buffer(data->event_path, mpack, mpack_size);
    // TODO: check rv?
}

static void
add_breadcrumb(sentry_backend_t *backend, sentry_value_t breadcrumb)
{
    crashpad_state_t *data = (crashpad_state_t *)backend->data;

    bool first_breadcrumb = data->num_breadcrumbs % SENTRY_BREADCRUMBS_MAX == 0;

    sentry_path_t *breadcrumb_file
        = data->num_breadcrumbs % (SENTRY_BREADCRUMBS_MAX * 2)
            < SENTRY_BREADCRUMBS_MAX
        ? data->breadcrumb1_path
        : data->breadcrumb2_path;

    size_t mpack_size;
    char *mpack = sentry_value_to_msgpack(breadcrumb, &mpack_size);

    int _rv = first_breadcrumb
        ? sentry__path_write_buffer(breadcrumb_file, mpack, mpack_size)
        : sentry__path_append_buffer(breadcrumb_file, mpack, mpack_size);
    // TODO: check rv?

    sentry_free(mpack);

    data->num_breadcrumbs++;
}

static void
backend_free(sentry_backend_t *backend)
{
    crashpad_state_t *data = (crashpad_state_t *)backend->data;
    sentry__path_free(data->event_path);
    sentry__path_free(data->breadcrumb1_path);
    sentry__path_free(data->breadcrumb2_path);
    sentry_free(data);
}

sentry_backend_t *
sentry__new_crashpad_backend(void)
{
    sentry_backend_t *backend = SENTRY_MAKE(sentry_backend_t);
    if (!backend) {
        return NULL;
    }
    crashpad_state_t *data = SENTRY_MAKE(crashpad_state_t);
    if (!data) {
        sentry_free(backend);
        return NULL;
    }
    data->num_breadcrumbs = 0;

    backend->startup_func = startup_crashpad_backend;
    backend->shutdown_func = NULL;
    backend->free_func = backend_free;
    backend->flush_scope_func = flush_scope;
    backend->add_breadcrumb_func = add_breadcrumb;
    backend->user_consent_changed_func = user_consent_changed;
    backend->data = data;

    return backend;
}
