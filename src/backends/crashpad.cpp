#include "crashpad.hpp"

#include <stdio.h>
#include <atomic>
#include <map>
#include <string>
#include <vector>
#include <mutex>

#include "../attachment.hpp"
#include "../internal.hpp"
#include "../options.hpp"
#include "../path.hpp"
#include "../value.hpp"

#include "client/crash_report_database.h"
#include "client/crashpad_client.h"
#include "client/crashpad_info.h"
#include "client/settings.h"

using namespace sentry;
using namespace backends;

class backends::CrashpadBackendImpl {
   public:
    CrashpadBackendImpl();

    Path event_filename;
    Path breadcrumb_filename;
    std::mutex breadcrumb_lock;
    int breadcrumb_fileid;
    int breadcrumbs_in_segment;
};

CrashpadBackendImpl::CrashpadBackendImpl() {
    breadcrumb_fileid = 0;
    breadcrumbs_in_segment = 0;
}

CrashpadBackend::CrashpadBackend()
    : m_impl(new backends::CrashpadBackendImpl()) {
}

CrashpadBackend::~CrashpadBackend() {
    delete m_impl;
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

    m_impl->event_filename = current_run_folder.join(SENTRY_EVENT_FILE_NAME);
    file_attachments.emplace(
        SENTRY_EVENT_FILE_ATTACHMENT_NAME,
        base::FilePath(
            current_run_folder.join(SENTRY_EVENT_FILE_NAME).as_osstr()));
    file_attachments.emplace(
        SENTRY_BREADCRUMB1_FILE_ATTACHMENT_NAME,
        base::FilePath(
            current_run_folder.join(SENTRY_BREADCRUMB1_FILE).as_osstr()));
    file_attachments.emplace(
        SENTRY_BREADCRUMB2_FILE_ATTACHMENT_NAME,
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

void CrashpadBackend::flush_scope_state(const sentry::Scope &scope) {
    mpack_writer_t writer;
    mpack_writer_init_stdfile(&writer, m_impl->event_filename.open("w"), true);
    Value event = Value::new_event();
    scope.applyToEvent(event, false);
    event.to_msgpack(&writer);
    mpack_error_t err = mpack_writer_destroy(&writer);
    if (err != mpack_ok) {
        SENTRY_LOGF("an error occurred encoding the data. Code: %d", err);
        return;
    }
}

void CrashpadBackend::add_breadcrumb(sentry::Value breadcrumb) {
    std::lock_guard<std::mutex> _blck(m_impl->breadcrumb_lock);
    const sentry_options_t *opts = sentry_get_options();

    if (m_impl->breadcrumbs_in_segment == 0 ||
        m_impl->breadcrumbs_in_segment == SENTRY_BREADCRUMBS_MAX) {
        m_impl->breadcrumb_fileid = m_impl->breadcrumb_fileid == 0 ? 1 : 0;
        m_impl->breadcrumbs_in_segment = 0;
        m_impl->breadcrumb_filename =
            opts->runs_folder.join(opts->run_id.c_str())
                .join(m_impl->breadcrumb_fileid == 0 ? SENTRY_BREADCRUMB1_FILE
                                                     : SENTRY_BREADCRUMB2_FILE);
    }

    std::string mpack = breadcrumb.to_msgpack();
    FILE *file = m_impl->breadcrumb_filename.open(
        m_impl->breadcrumbs_in_segment == 0 ? "w" : "a");
    if (file) {
        fwrite(mpack.c_str(), 1, mpack.size(), file);
        fclose(file);
    }

    m_impl->breadcrumbs_in_segment++;
}
