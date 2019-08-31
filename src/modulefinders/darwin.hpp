#ifndef SENTRY_MODULEFINDER_DARWIN_HPP_INCLUDED
#define SENTRY_MODULEFINDER_DARWIN_HPP_INCLUDED
#ifdef SENTRY_WITH_DARWIN_MODULE_FINDER

#include "../internal.hpp"
#include "../value.hpp"

namespace sentry {
namespace modulefinders {

Value get_darwin_module_list();

}  // namespace modulefinders
}  // namespace sentry

#endif
#endif
