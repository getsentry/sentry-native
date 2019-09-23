#ifndef SENTRY_BACKENDS_BREAKPAD_HPP_INCLUDED
#define SENTRY_BACKENDS_BREAKPAD_HPP_INCLUDED
#ifdef SENTRY_WITH_BREAKPAD_BACKEND
#include <cstdio>
#include <string>

#if defined(__APPLE__)
#include "client/mac/handler/exception_handler.h"
#elif defined(__linux__)
#include "client/linux/handler/exception_handler.h"
#elif defined(_WIN32)
#include "client/windows/handler/exception_handler.h"
#endif

#include "../attachment.hpp"
#include "../internal.hpp"
#include "../scope.hpp"
#include "base_backend.hpp"

namespace sentry {
namespace backends {

class BreakpadBackend : public Backend {
   public:
    BreakpadBackend();
    ~BreakpadBackend();

    void start();
    void flush_scope(const Scope &scope);
    void add_breadcrumb(Value breadcrumb);
    void dump_files();

   private:
    void upload_runs(const sentry_options_t *options) const;

    google_breakpad::ExceptionHandler *m_handler;
    const std::vector<Attachment> *m_attachments;

    // scope data
    std::mutex m_scope_lock;
    std::string m_scope_data;
    FILE *m_scope_file;

    // breadcrumb data
    std::mutex m_breadcrumbs_lock;
    std::vector<std::string> m_breadcrumbs_data;
    FILE *m_breadcrumbs_file;
};
}  // namespace backends
}  // namespace sentry

#endif
#endif
