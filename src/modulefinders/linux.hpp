#ifndef SENTRY_MODULEFINDER_LINUX_HPP_INCLUDED
#define SENTRY_MODULEFINDER_LINUX_HPP_INCLUDED
#ifdef SENTRY_WITH_LINUX_MODULE_FINDER

#include "base.hpp"

namespace sentry {
namespace modulefinders {

class LinuxModuleFinder : public ModuleFinder {
   public:
    LinuxModuleFinder();
    Value get_module_list() const;
};
}  // namespace modulefinders
}  // namespace sentry

#endif
#endif
