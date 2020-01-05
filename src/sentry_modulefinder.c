#include "sentry_modulefinder.h"

#if SENTRY_PLATFORM == SENTRY_PLATFORM_MACOS
#    include "darwin/sentry_darwin_modulefinder.h"
#endif

sentry_value_t
sentry__modules_get_list(void)
{
#if SENTRY_PLATFORM == SENTRY_PLATFORM_MACOS
    return sentry__darwin_modules_get_list();
#else
    return sentry_value_new_null();
#endif
}
