#ifdef SENTRY_WITH_CRASHPAD_BACKEND
#include <stdio.h>
#include <atomic>
#include <map>
#include <string>
#include <vector>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include "client/crash_report_database.h"
#include "client/crashpad_client.h"
#include "client/crashpad_info.h"
#include "client/settings.h"

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include "../attachment.hpp"
#include "../options.hpp"
#include "../value.hpp"

#include "crashpad_backend.hpp"

using namespace sentry;
using namespace backends;

static std::unique_ptr<crashpad::CrashReportDatabase> g_db;

CrashpadBackend::CrashpadBackend()
    : breadcrumb_fileid(0), breadcrumbs_in_segment(0) {
}

void CrashpadBackend::start() {
    const sentry_options_t *options = sentry_get_options();

    Path current_run_folder =
        options->runs_folder.join(options->run_id.c_str());
    current_run_folder.create_directories();

    base::FilePath database(options->database_path.as_osstr());
    base::FilePath handler(options->handler_path.as_osstr());

    std::map<std::string, std::string> annotations;
    std::map<std::string, base::FilePath> file_attachments;

    for (const sentry::Attachment &attachment : options->attachments) {
        file_attachments.emplace(attachment.name(),
                                 base::FilePath(attachment.path().as_osstr()));
    }

    event_filename = current_run_folder.join(SENTRY_EVENT_FILE);
    file_attachments.emplace(
        SENTRY_EVENT_FILE,
        base::FilePath(current_run_folder.join(SENTRY_EVENT_FILE).as_osstr()));

    // create both breadcrumbs files so that crashpad does not log an error
    // if they are missing.
    Path bc1 = current_run_folder.join(SENTRY_BREADCRUMBS1_FILE);
    bc1.touch();
    Path bc2 = current_run_folder.join(SENTRY_BREADCRUMBS2_FILE);
    bc2.touch();

    file_attachments.emplace(SENTRY_BREADCRUMBS1_FILE,
                             base::FilePath(bc1.as_osstr()));
    file_attachments.emplace(SENTRY_BREADCRUMBS2_FILE,
                             base::FilePath(bc2.as_osstr()));

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
    user_consent_changed();

    crashpad::CrashpadClient client;
    std::string url = options->dsn.get_minidump_url();
    bool success = client.StartHandlerWithAttachments(
        handler, database, database, url, annotations, file_attachments,
        arguments,
        /* restartable */ true,
        /* asynchronous_start */ false);

    if (success) {
        SENTRY_LOG("started client handler.");
    } else {
        SENTRY_LOG("failed to start client handler.");
        return;
    }

    if (!options->system_crash_reporter_enabled) {
        // Disable the system crash reporter. Especially on macOS, it takes
        // substantial time *after* crashpad has done its job.
        crashpad::CrashpadInfo *crashpad_info =
            crashpad::CrashpadInfo::GetCrashpadInfo();
        crashpad_info->set_system_crash_reporter_forwarding(
            crashpad::TriState::kDisabled);
    }
}

void CrashpadBackend::flush_scope(const sentry::Scope &scope) {
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
}

void CrashpadBackend::add_breadcrumb(sentry::Value breadcrumb) {
    std::lock_guard<std::mutex> _blck(breadcrumb_lock);
    const sentry_options_t *opts = sentry_get_options();

    if (breadcrumbs_in_segment == 0 ||
        breadcrumbs_in_segment == SENTRY_BREADCRUMBS_MAX) {
        breadcrumb_fileid = breadcrumb_fileid == 0 ? 1 : 0;
        breadcrumbs_in_segment = 0;
        breadcrumb_filename =
            opts->runs_folder.join(opts->run_id.c_str())
                .join(breadcrumb_fileid == 0 ? SENTRY_BREADCRUMBS1_FILE
                                             : SENTRY_BREADCRUMBS2_FILE);
    }

    size_t mpack_size;
    char *mpack = breadcrumb.to_msgpack_string(&mpack_size);
    FILE *file =
        breadcrumb_filename.open(breadcrumbs_in_segment == 0 ? "wb" : "a");
    if (file) {
        fwrite(mpack, 1, mpack_size, file);
        fclose(file);
    }
    free(mpack);

    breadcrumbs_in_segment++;
}

void CrashpadBackend::user_consent_changed() {
    if (!g_db || !g_db->GetSettings()) {
        return;
    }

    const sentry_options_t *opts = sentry_get_options();
    if (opts) {
        g_db->GetSettings()->SetUploadsEnabled(opts->should_upload());
    }
}

#endif
