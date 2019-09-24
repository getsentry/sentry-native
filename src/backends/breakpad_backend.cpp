#ifdef SENTRY_WITH_BREAKPAD_BACKEND
#include "../internal.hpp"
#include "../options.hpp"
#include "../path.hpp"

#include "breakpad_backend.hpp"

using namespace sentry;
using namespace backends;

bool callback_impl(bool succeeded, void *context) {
    if (succeeded) {
        SENTRY_LOG("Created minidump.");
    } else {
        SENTRY_LOG("Failed to create minidump.");
    }

    ((BreakpadBackend *)context)->dump_files();
    return succeeded;
}

#if defined(__APPLE__)
bool callback(const char *dump_dir,
              const char *minidump_id,
              void *context,
              bool succeeded) {
    return callback_impl(succeeded, context);
}
#elif defined(__linux__)
bool callback(const google_breakpad::MinidumpDescriptor &descriptor,
              void *context,
              bool succeeded) {
    return callback_impl(succeeded, context);
}
#elif defined(_WIN32)
bool callback(const wchar_t *dump_path,
              const wchar_t *minidump_id,
              void *context,
              EXCEPTION_POINTERS *exinfo,
              MDRawAssertionInfo *assertion,
              bool succeeded) {
    return callback_impl(succeeded, context);
}
#endif

BreakpadBackend::BreakpadBackend()
    : m_handler(nullptr), m_scope_file(nullptr), m_breadcrumbs_file(nullptr) {
}

BreakpadBackend::~BreakpadBackend() {
    if (m_handler) {
        delete m_handler;
    }
}

void BreakpadBackend::start() {
    if (m_handler != nullptr) {
        return;
    }

    const sentry_options_t *options = sentry_get_options();
    m_attachments = &options->attachments;

    const Path &run_path = options->runs_folder.join(options->run_id.c_str());
    run_path.create_directories();

    m_scope_file = run_path.join(SENTRY_EVENT_FILE).open("w");
    m_breadcrumbs_file = run_path.join(SENTRY_BREADCRUMB1_FILE).open("w");

    const auto *db_path = options->database_path.as_osstr();

#if defined(__APPLE__)
    m_handler =
        new google_breakpad::ExceptionHandler(db_path,
                                              /* filter */ nullptr,
                                              /* */ callback,
                                              /* context */ this,
                                              /* install handler */ true,
                                              /* port name */ nullptr);
#elif defined(__linux__)
    google_breakpad::MinidumpDescriptor descriptor(db_path);
    m_handler =
        new google_breakpad::ExceptionHandler(descriptor,
                                              /* filter */ nullptr,
                                              /* */ callback,
                                              /* context */ this,
                                              /* install handler */ true,
                                              /* server fd */ -1);
#elif defined(_WIN32)
    google_breakpad::ExceptionHandler::HandlerType handler_type =
        google_breakpad::ExceptionHandler::HANDLER_ALL;
    m_handler = new google_breakpad::ExceptionHandler(db_path,
                                                      /* filter */ nullptr,
                                                      /* */ callback,
                                                      /* context */ this,
                                                      /* */ handler_type);
#endif

    SENTRY_LOG("started client handler.");

    upload_runs(options);
}

void BreakpadBackend::upload_runs(const sentry_options_t *options) const {
    // Path pending_path = options->database_path.join(SENTRY_PENDING_FOLDER);
    // pending_path.create_directories();

    sentry::transports::Transport *transport = options->transport;
    if (!transport) {
        SENTRY_LOG("Skipping runs upload, no transport configured.");
        return;
    }

    for (PathIterator iter = options->runs_folder.iter_directory();
         iter.next();) {
        // TODO
    }
}

void BreakpadBackend::flush_scope(const sentry::Scope &scope) {
    std::lock_guard<std::mutex> _lck(m_scope_lock);
    Value event = Value::new_object();
    scope.apply_to_event(event, true);
    m_scope_data = event.to_msgpack();
}

void BreakpadBackend::add_breadcrumb(sentry::Value breadcrumb) {
    std::lock_guard<std::mutex> _lck(m_breadcrumbs_lock);
    auto &crumbs = m_breadcrumbs_data;
    size_t overhead = crumbs.size() - SENTRY_BREADCRUMBS_MAX + 1;
    crumbs.erase(crumbs.begin(), crumbs.end() + overhead);
    crumbs.push_back(breadcrumb.to_msgpack());
}

void BreakpadBackend::dump_files() {
    if (m_scope_file) {
        fwrite(m_scope_data.c_str(), 1, m_scope_data.size(), m_scope_file);
        fclose(m_scope_file);
        m_scope_file = nullptr;
    }

    if (m_breadcrumbs_file) {
        for (const std::string &b : m_breadcrumbs_data) {
            fwrite(b.c_str(), 1, b.size(), m_breadcrumbs_file);
        }

        fclose(m_breadcrumbs_file);
        m_breadcrumbs_file = nullptr;
    }

    for (const sentry::Attachment &attachment : *m_attachments) {
        // TODO: Copy attachment files
    }
}

#endif
