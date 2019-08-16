#include "base.hpp"
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

void Backend::flushScopeState(const Scope &scope) {
}

void Backend::addBreadcrumb(Value breadcrumb) {
}
