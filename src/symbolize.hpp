#ifndef SENTRY_SYMBOLIZER_BASE_HPP_INCLUDED
#define SENTRY_SYMBOLIZER_BASE_HPP_INCLUDED

#ifdef _WIN32
#define SENTRY_WITH_WINDOWS_SYMBOLIZER
#elif defined(__APPLE__) || defined(__linux__)
#define SENTRY_WITH_DLADDR_SYMBOLIZER
#endif

#include <functional>

#include "internal.hpp"

namespace sentry {
namespace symbolizers {

struct FrameInfo {
    void *load_addr;
    void *symbol_addr;
    void *instruction_addr;
    const char *symbol;
    const char *filename;
    const char *object_name;
    uint32_t lineno;
};

bool symbolize(void *addr, std::function<void(const FrameInfo *)> func);

}  // namespace symbolizers
}  // namespace sentry

#endif
