#include "sentry_symbolizer.h"

#ifdef SENTRY_PLATFORM_UNIX
#    include "unix/sentry_unix_symbolizer_dladdr.h"
#endif

bool
sentry__symbolize(
    void *addr, void (*func)(const sentry_frame_info_t *, void *), void *data)
{
#ifdef SENTRY_PLATFORM_UNIX
    return sentry__symbolize_dladdr(addr, func, data);
#else
    return false;
#endif
}
