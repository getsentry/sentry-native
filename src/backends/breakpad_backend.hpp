#ifndef SENTRY_BACKENDS_BREAKPAD_HPP_INCLUDED
#define SENTRY_BACKENDS_BREAKPAD_HPP_INCLUDED
#ifdef SENTRY_WITH_BREAKPAD_BACKEND

#include "../internal.hpp"
#include "../scope.hpp"
#include "base_backend.hpp"

namespace sentry {
namespace backends {

class BreakpadBackend : public Backend {
   public:
    BreakpadBackend();

    void start();
    void flush_scope(const sentry::Scope &scope);
    void add_breadcrumb(sentry::Value breadcrumb);

   private:
};
}  // namespace backends
}  // namespace sentry

#endif
#endif
