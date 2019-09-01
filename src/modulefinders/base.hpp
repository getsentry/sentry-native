#ifndef SENTRY_MODULEFINDER_BASE_HPP_INCLUDED
#define SENTRY_MODULEFINDER_BASE_HPP_INCLUDED

#ifdef _WIN32
#define SENTRY_WITH_WINDOWS_MODULEFINDER
#elif defined(__APPLE__)
#define SENTRY_WITH_DARWIN_MODULEFINDER
#elif defined(__linux__)
#define SENTRY_WITH_LINUX_MODULEFINDER
#endif

#include "../internal.hpp"
#include "../value.hpp"

namespace sentry {
namespace modulefinders {

Value get_module_list();

}  // namespace modulefinders
}  // namespace sentry

#endif
