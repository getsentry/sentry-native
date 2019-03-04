#if defined(SENTRY_CRASHPAD)
#include "crashpad_wrapper.hpp"
#elif defined(SENTRY_BREAKPAD)
#include "breakpad_wrapper.hpp"
#endif
#include "sentry.h"
#include <map>
#include <string>
#include "mpack.c"

#if defined(SENTRY_CRASHPAD)
using namespace sentry::crashpad;
#elif defined(SENTRY_BREAKPAD)
using namespace sentry::breakpad;
#endif

static const char *SENTRY_EVENT_FILE_NAME = "sentry-event.mp";

typedef struct sentry_event_s
{
    const char *release;
    sentry_level_t level;
    sentry_user_t *user;
    const char *dist;
    const char *environment;
    const char *transaction;
    std::map<std::string, std::string> tags;
    std::map<std::string, std::string> extra;

} sentry_event_t;

sentry_event_t sentry_event = {
    .release = nullptr,
    .level = SENTRY_LEVEL_ERROR,
    .user = nullptr,
    .dist = nullptr,
    .environment = nullptr,
    .transaction = nullptr,
    .tags = std::map<std::string, std::string>(),
    .extra = std::map<std::string, std::string>(),
};

void serialize(const sentry_event_t *event)
{
    mpack_writer_t writer;
    // TODO: cycle event file
    mpack_writer_init_filename(&writer, SENTRY_EVENT_FILE_NAME);
    mpack_start_map(&writer, 7);
    mpack_write_cstr(&writer, "release");
    mpack_write_cstr_or_nil(&writer, event->release);
    mpack_write_cstr(&writer, "level");
    mpack_write_int(&writer, event->level);
    if (event->user != nullptr)
    {
        // TODO
    }
    mpack_write_cstr(&writer, "dist");
    mpack_write_cstr_or_nil(&writer, event->dist);
    mpack_write_cstr(&writer, "environment");
    mpack_write_cstr_or_nil(&writer, event->environment);
    mpack_write_cstr(&writer, "transaction");
    mpack_write_cstr_or_nil(&writer, event->transaction);

    int tag_count = event->tags.size();
    mpack_write_cstr(&writer, "tags");
    mpack_start_map(&writer, tag_count); // tags
    if (tag_count > 0)
    {
        std::map<std::string, std::string>::const_iterator iter;
        for (iter = event->tags.begin(); iter != event->tags.end(); ++iter)
        {
            mpack_write_cstr(&writer, iter->first.c_str());
            mpack_write_cstr(&writer, iter->second.c_str());
        }
    }
    mpack_finish_map(&writer); // tags

    int extra_count = event->extra.size();
    mpack_write_cstr(&writer, "extra");
    mpack_start_map(&writer, extra_count); // extra
    if (extra_count > 0)
    {
        std::map<std::string, std::string>::const_iterator iter;
        for (iter = event->extra.begin(); iter != event->extra.end(); ++iter)
        {
            mpack_write_cstr(&writer, iter->first.c_str());
            mpack_write_cstr(&writer, iter->second.c_str());
        }
    }
    mpack_finish_map(&writer); // extra

    mpack_finish_map(&writer); // root

    if (mpack_writer_destroy(&writer) != mpack_ok)
    {
        fprintf(stderr, "An error occurred encoding the data!\n");
        return;
    }
    // atomic move on event file
    // breadcrumb will send send both files
}

int sentry_init(const sentry_options_t *options)
{
    auto err = init(options);
    if (err != 0)
    {
        return err;
    }

    if (options->environment != nullptr)
    {
        sentry_event.environment = options->environment;
    }

    if (options->release != nullptr)
    {
        sentry_event.release = options->release;
    }

    if (options->dist != nullptr)
    {
        sentry_event.dist = options->dist;
    }

    return SENTRY_ERROR_NULL_ARGUMENT;
}

void sentry_options_init(sentry_options_t *options)
{
}

// int sentry_add_breadcrumb(sentry_breadcrumb_t *breadcrumb);
// int sentry_push_scope();
// int sentry_pop_scope();
int sentry_set_user(sentry_user_t *user);
int sentry_remove_user();
int sentry_set_fingerprint(const char **fingerprint, size_t len);
int sentry_remove_fingerprint();

int sentry_set_level(enum sentry_level_t level)
{
    sentry_event.level = level;
    serialize(&sentry_event);
    return SENTRY_SUCCESS;
}

int sentry_set_transaction(const char *transaction)
{
    sentry_event.transaction = transaction;
    serialize(&sentry_event);
    return SENTRY_SUCCESS;
}

int sentry_remove_transaction()
{
    sentry_set_transaction(nullptr);
}

int sentry_set_user(sentry_user_t *user)
{
    sentry_event.user = user;
    serialize(&sentry_event);
    return SENTRY_SUCCESS;
}

int sentry_remove_user()
{
    int rv = sentry_set_transaction(nullptr);
    serialize(&sentry_event);
    return rv;
}

int sentry_set_tag(const char *key, const char *value)
{
    sentry_event.tags[key] = value;
    serialize(&sentry_event);
    return SENTRY_SUCCESS;
}

int sentry_remove_tag(const char *key)
{
    sentry_event.tags.erase(key);
    serialize(&sentry_event);
    return SENTRY_SUCCESS;
}

int sentry_set_extra(const char *key, const char *value)
{
    sentry_event.extra[key] = value;
    serialize(&sentry_event);
    return SENTRY_SUCCESS;
}

int sentry_remove_extra(const char *key)
{
    sentry_event.extra.erase(key);
    serialize(&sentry_event);
    return SENTRY_SUCCESS;
}

int sentry_set_release(const char *release)
{
    sentry_event.release = release;
    serialize(&sentry_event);
    return SENTRY_SUCCESS;
}

int sentry_remove_release()
{
    int rv = sentry_set_release(nullptr);
    serialize(&sentry_event);
    return rv;
}
