#ifndef SENTRY_BACKENDS_BASE_HPP_INCLUDED
#define SENTRY_BACKENDS_BASE_HPP_INCLUDED

#include "../internal.hpp"
#include "../scope.hpp"
#include "../value.hpp"

namespace sentry {
namespace backends {
class Backend {
   public:
    Backend();
    virtual ~Backend();
    virtual void start();
    virtual void shutdown();
    virtual void flush_scope_state(const sentry::Scope &scope);
    virtual void add_breadcrumb(sentry::Value breadcrumb);

   private:
    Backend(const Backend &) = delete;
    Backend &operator=(Backend &) = delete;
};
}  // namespace backends
}  // namespace sentry

#endif
