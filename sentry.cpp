#if defined(SENTRY_CRASHPAD)
#include "crashpad_wrapper.hpp"
#elif defined(SENTRY_BREAKPAD)
#include "breakpad_wrapper.hpp"
#endif
#include "sentry.h"
#include <string>

#if defined(SENTRY_CRASHPAD)
using namespace sentry::crashpad;
#elif defined(SENTRY_BREAKPAD)
using namespace sentry::breakpad;
#endif

int sentry_init(const sentry_options_t *options)
{
    auto rv = init(options);
    if (rv != 1)
    {
        return rv;
    }

    if (options->environment != nullptr)
    {
        set_annotation("sentry[environment]", options->environment);
    }

    if (options->release != nullptr)
    {
        set_annotation("sentry[release]", options->release);
    }

    if (options->dist != nullptr)
    {
        set_annotation("sentry[dist]", options->dist);
    }
}

// int sentry_add_breadcrumb(sentry_breadcrumb_t *breadcrumb);
// int sentry_push_scope();
// int sentry_pop_scope();
int sentry_set_user(sentry_user_t *user);
int sentry_remove_user();
int sentry_set_fingerprint(const char **fingerprint, size_t len);
int sentry_remove_fingerprint();
int sentry_set_transaction(const char *transaction);
int sentry_remove_transaction();
int sentry_set_level(enum sentry_level_t level)
{
}

int sentry_set_tag(const char *key, const char *value)
{
    std::string string_key(key);
    std::string final_key = "sentry[tags][" + string_key + "]";
    return set_annotation(final_key.c_str(), value);
}

int sentry_remove_tag(const char *key)
{
    std::string string_key(key);
    std::string final_key = "sentry[tags][" + string_key + "]";
    return remove_annotation(final_key.c_str());
}

int sentry_set_extra(const char *key, const char *value)
{
    std::string string_key(key);
    std::string final_key = "sentry[extra][" + string_key + "]";
    return set_annotation(final_key.c_str(), value);
}

int sentry_remove_extra(const char *key)
{
    std::string string_key(key);
    std::string final_key = "sentry[tags][" + string_key + "]";
    return remove_annotation(final_key.c_str());
}

int sentry_set_release(const char *release)
{
    return set_annotation("sentry[release]", release);
}

int sentry_remove_release()
{
    return remove_annotation("sentry[release]");
}
