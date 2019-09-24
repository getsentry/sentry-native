#ifndef SENTRY_BACKENDS_CRASHPAD_HPP_INCLUDED
#define SENTRY_BACKENDS_CRASHPAD_HPP_INCLUDED
#ifdef SENTRY_WITH_CRASHPAD_BACKEND
#include <mutex>

#include "../internal.hpp"
#include "../path.hpp"
#include "../scope.hpp"
#include "base_backend.hpp"

namespace sentry {
namespace backends {

class CrashpadBackend : public Backend {
   public:
    CrashpadBackend();

    void start();
    void flush_scope(const Scope &scope);
    void add_breadcrumb(Value breadcrumb);

   private:
    Path event_filename;
    Path breadcrumb_filename;
    std::mutex breadcrumb_lock;
    int breadcrumb_fileid;
    int breadcrumbs_in_segment;
};
}  // namespace backends
}  // namespace sentry

#endif
#endif
