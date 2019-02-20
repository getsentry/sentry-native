#include "crashpad_wrapper.hpp"
#include "sentrypad.h"
#include <string>

#if defined(SENTRY_CRASHPAD)
namespace implpad = sentry::crashpad;
#elif defined(SENTRY_BREAKPAD)
namespace implpad = sentry::breakpad;
#endif

int sentrypad_init()
{
    return implpad::init();
}

int sentrypad_set_tag(const char *key, const char *value)
{
    std::string string_key(key);
    std::string final_key = "sentry[tags][" + string_key + "]";
    return implpad::set_annotation(final_key.c_str(), value);
}

int sentrypad_set_extra(const char *key, const char *value)
{
    std::string string_key(key);
    std::string final_key = "sentry[extra][" + string_key + "]";
    return implpad::set_annotation(final_key.c_str(), value);
}

int sentrypad_set_release(const char *release)
{
    return implpad::set_annotation("sentry[release]", release);
}
