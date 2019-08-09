#ifndef SENTRY_SCOPE_HPP_INCLUDED
#define SENTRY_SCOPE_HPP_INCLUDED

#include <string>
#include <unordered_map>
#include <vector>
#include "internal.hpp"
#include "protocol_value.hpp"

namespace sentry {
struct Scope {
    Scope()
        : level(SENTRY_LEVEL_ERROR),
          extra(Value::newObject()),
          tags(Value::newObject()),
          fingerprint(Value::newList()) {
    }

    Value createEvent();

    std::string transaction;
    sentry::Value fingerprint;
    sentry::Value user;
    sentry::Value tags;
    sentry::Value extra;
    sentry_level_t level;
};
}  // namespace sentry

#endif
