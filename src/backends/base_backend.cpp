#include "base_backend.hpp"
#include "../scope.hpp"

using namespace sentry;
using namespace backends;

Backend::Backend() {
}

Backend::~Backend() {
}

void Backend::start() {
}

void Backend::shutdown() {
}

void Backend::flush_scope_state(const Scope &scope) {
}

void Backend::add_breadcrumb(Value breadcrumb) {
}
