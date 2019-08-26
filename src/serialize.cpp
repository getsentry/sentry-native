#include "serialize.hpp"
#include <ctime>
#include "options.hpp"

static const char *level_as_string(sentry_level_t level) {
    switch (level) {
        case SENTRY_LEVEL_DEBUG:
            return "debug";
        case SENTRY_LEVEL_WARNING:
            return "warning";
        case SENTRY_LEVEL_ERROR:
            return "error";
        case SENTRY_LEVEL_FATAL:
            return "fatal";
        case SENTRY_LEVEL_INFO:
        default:
            return "info";
    }
}

static const char *cstr_or_null(const char *s) {
    return (s && *s) ? s : nullptr;
}

namespace sentry {

void serialize_breadcrumb(const sentry_breadcrumb_t *breadcrumb,
                          mpack_writer_t *writer) {
    mpack_start_map(writer, 5);
    mpack_write_cstr(writer, "timestamp");
    time_t now;
    time(&now);
    char buf[255];
    strftime(buf, sizeof buf, "%FT%TZ", gmtime(&now));
    mpack_write_cstr_or_nil(writer, buf);
    mpack_write_cstr(writer, "message");
    mpack_write_cstr_or_nil(writer, breadcrumb->message);
    mpack_write_cstr(writer, "type");
    mpack_write_cstr_or_nil(writer, breadcrumb->type);
    mpack_write_cstr(writer, "category");
    mpack_write_cstr_or_nil(writer, breadcrumb->category);
    mpack_write_cstr(writer, "level");
    mpack_write_cstr(writer, level_as_string(breadcrumb->level));
    mpack_finish_map(writer);
}

void serialize_scope_as_event(const sentry::Scope *scope,
                              mpack_writer_t *writer) {
    const sentry_options_t *options = sentry_get_options();
    mpack_start_map(writer, 10);
    mpack_write_cstr(writer, "release");
    mpack_write_cstr_or_nil(writer, cstr_or_null(options->release.c_str()));
    mpack_write_cstr(writer, "level");
    mpack_write_cstr(writer, level_as_string(scope->level));

    mpack_write_cstr(writer, "user");
    if (!scope->user.empty()) {
        mpack_start_map(writer, (uint32_t)scope->user.size());
        std::unordered_map<std::string, std::string>::const_iterator iter;
        for (iter = scope->user.begin(); iter != scope->user.end(); ++iter) {
            mpack_write_cstr(writer, iter->first.c_str());
            mpack_write_cstr_or_nil(writer, iter->second.c_str());
        }
        mpack_finish_map(writer);
    } else {
        mpack_write_nil(writer);
    }

    mpack_write_cstr(writer, "dist");
    mpack_write_cstr_or_nil(writer, cstr_or_null(options->dist.c_str()));
    mpack_write_cstr(writer, "environment");
    mpack_write_cstr_or_nil(writer, cstr_or_null(options->environment.c_str()));
    mpack_write_cstr(writer, "transaction");
    mpack_write_cstr_or_nil(writer, cstr_or_null(scope->transaction.c_str()));

    size_t tag_count = scope->tags.size();
    mpack_write_cstr(writer, "tags");
    mpack_start_map(writer, (uint32_t)tag_count);
    if (tag_count > 0) {
        std::unordered_map<std::string, std::string>::const_iterator iter;
        for (iter = scope->tags.begin(); iter != scope->tags.end(); ++iter) {
            mpack_write_cstr(writer, iter->first.c_str());
            mpack_write_cstr_or_nil(writer, iter->second.c_str());
        }
    }
    mpack_finish_map(writer);

    size_t extra_count = scope->extra.size();
    mpack_write_cstr(writer, "extra");
    mpack_start_map(writer, (uint32_t)extra_count);
    if (extra_count > 0) {
        std::unordered_map<std::string, std::string>::const_iterator iter;
        for (iter = scope->extra.begin(); iter != scope->extra.end(); ++iter) {
            mpack_write_cstr(writer, iter->first.c_str());
            mpack_write_cstr_or_nil(writer, iter->second.c_str());
        }
    }
    mpack_finish_map(writer);

    size_t fingerprint_count = scope->fingerprint.size();
    mpack_write_cstr(writer, "fingerprint");
    mpack_start_array(writer, (uint32_t)fingerprint_count);
    if (fingerprint_count > 0) {
        for (const std::string &part : scope->fingerprint) {
            mpack_write_cstr_or_nil(writer, part.c_str());
        }
    }
    mpack_finish_array(writer);

    mpack_write_cstr(writer, "sdk");
    mpack_start_map(writer, 3);
    mpack_write_cstr(writer, "name");
    mpack_write_cstr(writer, "sentry.native");
    mpack_write_cstr(writer, "version");
    mpack_write_cstr(writer, SENTRY_SDK_VERSION);
    mpack_write_cstr(writer, "packages");
    mpack_start_array(writer, 1);
    mpack_start_map(writer, 2);
    mpack_write_cstr(writer, "name");
    mpack_write_cstr(writer, "github:getsentry/sentry-native");
    mpack_write_cstr(writer, "version");
    mpack_write_cstr(writer, SENTRY_SDK_VERSION);
    mpack_finish_map(writer);
    mpack_finish_array(writer);
    mpack_finish_map(writer);

    mpack_finish_map(writer);
}

}  // namespace sentry
