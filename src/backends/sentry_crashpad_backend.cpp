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

static std::unique_ptr<crashpad::CrashReportDatabase> g_db;

static void
user_consent_changed(struct sentry_backend_s *)
{
    if (!g_db || !g_db->GetSettings()) {
        return;
    }
    g_db->GetSettings()->SetUploadsEnabled(!sentry__should_skip_upload());
}

static void
startup_crashpad_backend(sentry_backend_t *backend)
{
    const sentry_options_t *options = sentry_get_options();
    if (!options->handler_path
        || !sentry__path_is_file(options->handler_path)) {
        SENTRY_DEBUG("unable to start crashpad backend, invalid handler_path");
        return;
    }

    SENTRY_TRACE("starting crashpad backend");
    sentry_path_t *current_run_folder = options->run->run_path;

    base::FilePath database(options->database_path->path);
    base::FilePath handler(options->handler_path->path);

    std::map<std::string, std::string> annotations;
    std::map<std::string, base::FilePath> file_attachments;

    /*for (const sentry::Attachment &attachment : options->attachments) {
        file_attachments.emplace(
            attachment.name(), base::FilePath(attachment.path().as_osstr()));
    }

    event_filename = current_run_folder.join(SENTRY_EVENT_FILE);
    file_attachments.emplace(SENTRY_EVENT_FILE,
        base::FilePath(current_run_folder.join(SENTRY_EVENT_FILE).as_osstr()));

    // create both breadcrumbs files so that crashpad does not log an error
    // if they are missing.
    Path bc1 = current_run_folder.join(SENTRY_BREADCRUMBS1_FILE);
    bc1.touch();
    Path bc2 = current_run_folder.join(SENTRY_BREADCRUMBS2_FILE);
    bc2.touch();

    file_attachments.emplace(
        SENTRY_BREADCRUMBS1_FILE, base::FilePath(bc1.as_osstr()));
    file_attachments.emplace(
        SENTRY_BREADCRUMBS2_FILE, base::FilePath(bc2.as_osstr()));
*/
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
flush_scope(struct sentry_backend_s *, const sentry_scope_t *scope)
{
    /*
    // if we can't open the file, fail silently.
    FILE *f = event_filename.open("wb");
    if (!f) {
        return;
    }
    mpack_writer_t writer;
    mpack_writer_init_stdfile(&writer, f, true);
    Value event = Value::new_object();
    scope.apply_to_event(event, SENTRY_SCOPE_NONE);
    event.to_msgpack(&writer);
    mpack_error_t err = mpack_writer_destroy(&writer);
    if (err != mpack_ok) {
        SENTRY_LOGF("an error occurred encoding the data. Code: %d", err);
        return;
    }
    */
}

static void
add_breadcrumb(struct sentry_backend_s *, sentry_value_t breadcrumb)
{
    /*
    std::lock_guard<std::mutex> _blck(breadcrumb_lock);
    const sentry_options_t *opts = sentry_get_options();

    if (breadcrumbs_in_segment == 0
        || breadcrumbs_in_segment == SENTRY_BREADCRUMBS_MAX) {
        breadcrumb_fileid = breadcrumb_fileid == 0 ? 1 : 0;
        breadcrumbs_in_segment = 0;
        breadcrumb_filename
            = opts->runs_folder.join(opts->run_id.c_str())
                  .join(breadcrumb_fileid == 0 ? SENTRY_BREADCRUMBS1_FILE
                                               : SENTRY_BREADCRUMBS2_FILE);
    }

    size_t mpack_size;
    char *mpack = breadcrumb.to_msgpack_string(&mpack_size);
    FILE *file
        = breadcrumb_filename.open(breadcrumbs_in_segment == 0 ? "wb" : "a");
    if (file) {
        fwrite(mpack, 1, mpack_size, file);
        fclose(file);
    }
    free(mpack);

    breadcrumbs_in_segment++;
    */
}

sentry_backend_t *
sentry__new_crashpad_backend(void)
{
    sentry_backend_t *backend = SENTRY_MAKE(sentry_backend_t);
    if (!backend) {
        return NULL;
    }

    backend->startup_func = startup_crashpad_backend;
    backend->shutdown_func = NULL;
    backend->free_func = NULL;
    backend->flush_scope_func = flush_scope;
    backend->add_breadcrumb_func = add_breadcrumb;
    backend->user_consent_changed_func = user_consent_changed;
    backend->data = NULL;

    return backend;
}
