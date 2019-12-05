#ifndef SENTRY_SCOPE_HPP_INCLUDED
#define SENTRY_SCOPE_HPP_INCLUDED

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "internal.hpp"
#include "value.hpp"

namespace sentry {
enum ScopeMode {
    SENTRY_SCOPE_NONE = 0x0,
    SENTRY_SCOPE_BREADCRUMBS = 0x1,
    SENTRY_SCOPE_MODULES = 0x2,
    SENTRY_SCOPE_STACKTRACES = 0x4,
    SENTRY_SCOPE_ALL = 0x7,
};

struct Scope {
    Scope()
        : fingerprint(Value::new_list()),
          tags(Value::new_object()),
          extra(Value::new_object()),
          contexts(Value::new_object()),
          breadcrumbs(Value::new_list()),
          level(SENTRY_LEVEL_ERROR) {
    }

    static void with_scope(std::function<void(const Scope &)> func);
    static void with_scope_mut(std::function<void(Scope &)> func);

    void apply_to_event(Value &event, ScopeMode mode) const;
    void apply_to_event(Value &event) const {
        apply_to_event(event, SENTRY_SCOPE_ALL);
    }

    std::string transaction;
    sentry::Value fingerprint;
    sentry::Value user;
    sentry::Value tags;
    sentry::Value extra;
    sentry::Value contexts;
    sentry::Value breadcrumbs;
    sentry_level_t level;
};
}  // namespace sentry

#endif
