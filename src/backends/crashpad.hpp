#ifndef SENTRY_BACKENDS_CRASHPAD_HPP_INCLUDED
#define SENTRY_BACKENDS_CRASHPAD_HPP_INCLUDED

#include "../internal.hpp"
#include "../scope.hpp"
#include "base.hpp"

namespace sentry {
namespace backends {

class CrashpadBackendImpl;

class CrashpadBackend : public Backend {
   public:
    CrashpadBackend();
    ~CrashpadBackend();
    void start();
    void flushScopeState(const sentry::Scope &scope);
    void addBreadcrumb(sentry::Value breadcrumb);

   private:
    CrashpadBackendImpl *m_impl;
};
}  // namespace backends
}  // namespace sentry

#endif
