#include "base.hpp"
#ifdef SENTRY_WITH_WINDOWS_SYMBOLIZER

using namespace sentry;
using namespace symbolizers;

bool symbolizers::symbolize(void *addr,
                            std::function<void(const FrameInfo *)> func) {
    return false;
}

#endif
