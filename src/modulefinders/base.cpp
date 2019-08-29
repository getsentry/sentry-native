#include "base.hpp"

using namespace sentry;
using namespace modulefinders;

ModuleFinder::ModuleFinder() {
}

ModuleFinder::~ModuleFinder() {
}

Value ModuleFinder::get_module_list() const {
    return Value::new_list();
}
