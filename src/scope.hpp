#ifndef SENTRY_SCOPE_HPP_INCLUDED
#define SENTRY_SCOPE_HPP_INCLUDED

#include <string>
#include <unordered_map>
#include <vector>
#include "internal.hpp"
#include "value.hpp"

namespace sentry {
struct Scope {
    Scope()
        : level(SENTRY_LEVEL_ERROR),
          extra(Value::new_object()),
          tags(Value::new_object()),
          breadcrumbs(Value::new_list()),
          fingerprint(Value::new_list()) {
    }

    void apply_to_event(Value &event, bool with_breadcrumbs) const;
    void apply_to_event(Value &event) const {
        apply_to_event(event, true);
    }

    std::string transaction;
    sentry::Value fingerprint;
    sentry::Value user;
    sentry::Value tags;
    sentry::Value extra;
    sentry::Value breadcrumbs;
    sentry_level_t level;
};
}  // namespace sentry

#endif
