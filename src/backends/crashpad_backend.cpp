#ifdef SENTRY_WITH_CRASHPAD_BACKEND
#include <atomic>
#include <cstdio>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "client/crash_report_database.h"
#include "client/crashpad_client.h"
#include "client/crashpad_info.h"
#include "client/settings.h"

#include "../attachment.hpp"
#include "../internal.hpp"
#include "../options.hpp"
#include "../path.hpp"
#include "../value.hpp"

#include "crashpad_backend.hpp"

using namespace sentry;
using namespace backends;

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
    file_attachments.emplace(SENTRY_EVENT_FILE,
                             base::FilePath(event_filename.as_osstr()));
    file_attachments.emplace(
        SENTRY_BREADCRUMB1_FILE,
        base::FilePath(
            current_run_folder.join(SENTRY_BREADCRUMB1_FILE).as_osstr()));
    file_attachments.emplace(
        SENTRY_BREADCRUMB2_FILE,
        base::FilePath(
            current_run_folder.join(SENTRY_BREADCRUMB2_FILE).as_osstr()));

    std::vector<std::string> arguments;
    arguments.push_back("--no-rate-limit");

#ifdef _WIN32
    // Temporary fix for Windows
    arguments.push_back("--no-upload-gzip");
#endif

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

    std::unique_ptr<crashpad::CrashReportDatabase> db =
        crashpad::CrashReportDatabase::Initialize(database);

    if (db != nullptr && db->GetSettings() != nullptr) {
        db->GetSettings()->SetUploadsEnabled(true);
    }
}

void CrashpadBackend::flush_scope(const sentry::Scope &scope) {
    mpack_writer_t writer;
    mpack_writer_init_stdfile(&writer, event_filename.open("w"), true);
    Value event = Value::new_object();
    scope.apply_to_event(event, false);
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
                .join(breadcrumb_fileid == 0 ? SENTRY_BREADCRUMB1_FILE
                                             : SENTRY_BREADCRUMB2_FILE);
    }

    std::string mpack = breadcrumb.to_msgpack();
    FILE *file =
        breadcrumb_filename.open(breadcrumbs_in_segment == 0 ? "w" : "a");
    if (file) {
        fwrite(mpack.c_str(), 1, mpack.size(), file);
        fclose(file);
    }

    breadcrumbs_in_segment++;
}
#endif
