#include "breakpad.hpp"
#ifdef SENTRY_WITH_BREAKPAD_BACKEND

using namespace sentry;
using namespace backends;

class backends::BreakpadBackendImpl {
   public:
    BreakpadBackendImpl();
};

BreakpadBackendImpl::BreakpadBackendImpl() {
}

BreakpadBackend::BreakpadBackend()
    : m_impl(new backends::BreakpadBackendImpl()) {
}

BreakpadBackend::~BreakpadBackend() {
    delete m_impl;
}

void BreakpadBackend::start() {
}

void BreakpadBackend::flush_scope_state(const sentry::Scope &scope) {
}

void BreakpadBackend::add_breadcrumb(sentry::Value breadcrumb) {
}

#endif
