#include "breakpad_backend.hpp"
#ifdef SENTRY_WITH_BREAKPAD_BACKEND

using namespace sentry;
using namespace backends;

BreakpadBackend::BreakpadBackend() {
}

void BreakpadBackend::start() {
}

void BreakpadBackend::flush_scope(const sentry::Scope &scope) {
}

void BreakpadBackend::add_breadcrumb(sentry::Value breadcrumb) {
}

#endif
