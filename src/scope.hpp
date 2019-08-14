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
          extra(Value::newObject()),
          tags(Value::newObject()),
          breadcrumbs(Value::newList()),
          fingerprint(Value::newList()) {
    }

    void applyToEvent(Value &event);

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
