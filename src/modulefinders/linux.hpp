#ifndef SENTRY_MODULEFINDER_LINUX_HPP_INCLUDED
#define SENTRY_MODULEFINDER_LINUX_HPP_INCLUDED
#ifdef SENTRY_WITH_LINUX_MODULE_FINDER

#include "../internal.hpp"
#include "../value.hpp"

namespace sentry {
namespace modulefinders {

Value get_linux_module_list();
}
}  // namespace sentry

#endif
#endif
