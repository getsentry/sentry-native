#include "sentry_modulefinder.h"

#ifdef SENTRY_PLATFORM_DARWIN
#    include "darwin/sentry_darwin_modulefinder.h"
#endif
#ifdef SENTRY_PLATFORM_WINDOWS
#    include "windows/sentry_windows_modulefinder.h"
#endif

#define TRY_MODULEFINDER(Func)                                                 \
    do {                                                                       \
        sentry_value_t rv = Func();                                            \
        if (!sentry_value_is_null(rv)) {                                       \
            return rv;                                                         \
        }                                                                      \
    } while (0)

sentry_value_t
sentry__modules_get_list(void)
{
#ifdef SENTRY_PLATFORM_DARWIN
    TRY_MODULEFINDER(sentry__darwin_modules_get_list);
#endif
#ifdef SENTRY_PLATFORM_WINDOWS
    TRY_MODULEFINDER(sentry__windows_modules_get_list);
#endif
    return sentry_value_new_null();
}
