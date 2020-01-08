#include "sentry_symbolizer.h"

#ifdef SENTRY_PLATFORM_UNIX
#    include "unix/sentry_unix_symbolizer_dladdr.h"
#    define HAVE_DLADDR
#endif
#ifdef SENTRY_PLATFORM_WINDOWS
#    include "windows/sentry_windows_symbolizer_dbghelp.h"
#    define HAVE_DBGHELP
#endif

bool
sentry__symbolize(
    void *addr, void (*func)(const sentry_frame_info_t *, void *), void *data)
{
#ifdef HAVE_DLADDR
    if (sentry__symbolize_dladdr(addr, func, data)) {
        return true;
    }
#endif
#ifdef HAVE_DBGHELP
    if (sentry__symbolize_dbghelp(addr, func, data)) {
        return true;
    }
#endif
    return false;
}
