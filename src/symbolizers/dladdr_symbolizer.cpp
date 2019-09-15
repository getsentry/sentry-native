#include "../symbolize.hpp"
#ifdef SENTRY_WITH_DLADDR_SYMBOLIZER
#include <dlfcn.h>

using namespace sentry;
using namespace symbolizers;

bool symbolizers::symbolize(void *addr,
                            std::function<void(const FrameInfo *)> func) {
    Dl_info info;
    if (dladdr(addr, &info) == 0) {
        return false;
    }

    FrameInfo frame_info = {0};
    frame_info.load_addr = info.dli_fbase;
    frame_info.symbol_addr = info.dli_saddr;
    frame_info.instruction_addr = addr;
    frame_info.symbol = info.dli_sname;
    frame_info.object_name = info.dli_fname;
    func(&frame_info);
    return true;
}

#endif
