#include "sentry_modulefinder.h"

#ifdef SENTRY_PLATFORM_DARWIN
#    include "darwin/sentry_darwin_modulefinder.h"
#endif

sentry_value_t
sentry__modules_get_list(void)
{
#ifdef SENTRY_PLATFORM_DARWIN
    return sentry__darwin_modules_get_list();
#else
    return sentry_value_new_null();
#endif
}
