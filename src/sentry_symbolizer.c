#include "sentry_symbolizer.h"

#if SENTRY_PLATFORM != SENTRY_PLATFORM_WINDOWS
#    include "unix/sentry_unix_symbolizer_dladdr.h"
#endif

bool
sentry__symbolize(
    void *addr, void (*func)(const sentry_frame_info_t *, void *), void *data)
{
#if SENTRY_PLATFORM != SENTRY_PLATFORM_WINDOWS
    return sentry__symbolize_dladdr(addr, func, data);
#else
    return false;
#endif
}
