#ifndef SENTRY_MODULEFINDER_DARWIN_HPP_INCLUDED
#define SENTRY_MODULEFINDER_DARWIN_HPP_INCLUDED
#ifdef SENTRY_WITH_DARWIN_MODULE_FINDER

#include "base.hpp"

namespace sentry {
namespace modulefinders {

class DarwinModuleFinder : public ModuleFinder {
   public:
    DarwinModuleFinder();
    Value get_module_list() const;
};
}  // namespace modulefinders
}  // namespace sentry

#endif
#endif
