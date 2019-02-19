#include "crashpad_wrapper.hpp"
#include "sentrypad.h"
#include <string>

#ifdef SENTRY_CRASHPAD
namespace implpad = sentry::crashpad;
#elif SENTRY_BREAKPAD
namespace implpad = sentry::crashpad;
#endif

int sentrypad_init()
{
    return implpad::init();
}

int sentrypad_set_tag(const char *key, const char *value)
{
    // std::string string_key(key);
    // std::string final_key = "sentry[" + string_key + "]";
    return implpad::set_tag(key, value);
}

int sentrypad_set_extra(const char *key, const char *value)
{
    return implpad::set_extra(key, value);
}

int sentrypad_set_release(const char *release)
{
    return implpad::set_tag("sentry[release]", release);
}
