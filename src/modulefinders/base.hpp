#ifndef SENTRY_MODULEFINDER_BASE_HPP_INCLUDED
#define SENTRY_MODULEFINDER_BASE_HPP_INCLUDED

#include <string>
#include "../internal.hpp"
#include "../value.hpp"

namespace sentry {
namespace modulefinders {

class ModuleFinder {
   public:
    ModuleFinder();
    virtual ~ModuleFinder();
    virtual Value get_module_list() const;

   private:
    ModuleFinder(const ModuleFinder &) = delete;
    ModuleFinder &operator=(ModuleFinder &) = delete;
};
}  // namespace modulefinders
}  // namespace sentry

#endif
