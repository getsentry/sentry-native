#ifndef SENTRY_SERIALIZE_HPP_INCLUDED
#define SENTRY_SERIALIZE_HPP_INCLUDED

#include "internal.hpp"
#include "scope.hpp"
#include "vendor/mpack.h"

namespace sentry {

void serialize_breadcrumb(const sentry_breadcrumb_t *breadcrumb,
                          mpack_writer_t *writer);
void serialize_scope_as_event(const sentry::Scope *scope,
                              mpack_writer_t *writer);
}  // namespace sentry

#endif
