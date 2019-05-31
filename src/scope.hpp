#ifndef SENTRY_SCOPE_HPP_INCLUDED
#define SENTRY_SCOPE_HPP_INCLUDED

#include <string>
#include <unordered_map>
#include <vector>
#include "internal.hpp"

namespace sentry {
struct Scope {
    Scope() : level(SENTRY_LEVEL_ERROR) {
    }
    std::string transaction;
    std::vector<std::string> fingerprint;
    std::unordered_map<std::string, std::string> user;
    std::unordered_map<std::string, std::string> tags;
    std::unordered_map<std::string, std::string> extra;
    sentry_level_t level;
};
}  // namespace sentry

#endif
