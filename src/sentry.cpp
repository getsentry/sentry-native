#if defined(SENTRY_CRASHPAD)
#include "crashpad_wrapper.hpp"
#elif defined(SENTRY_BREAKPAD)
#include "breakpad_wrapper.hpp"
#endif
#include <stdarg.h>
#include <sys/stat.h>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include "ctime"
#include "internal.hpp"
#include "macros.hpp"
#include "random"
#include "sentry.h"
#include "vendor/mpack.h"

#if defined(SENTRY_CRASHPAD)
using namespace sentry::crashpad;
#elif defined(SENTRY_BREAKPAD)
using namespace sentry::breakpad;
#endif

#define BEGIN_MODIFY_EVENT std::lock_guard<std::mutex> _lck(event_lock)
#define END_MODIFY_EVENT serialize(&sentry_event)
#define BEGIN_MODIFY_BREADCRUMB \
    std::lock_guard<std::mutex> _lck(breadcrumb_lock)
#define END_MODIFY_BREADCRUMB

struct SentryDsn {
    const char *scheme;
    const char *public_key;
    const char *private_key;
    const char *host;
    const char *path;
    const char *project_id;
};

struct SentryEvent {
    const char *release;
    sentry_level_t level;
    const char *dist;
    const char *environment;
    const char *transaction;
    std::map<std::string, std::string> user;
    std::map<std::string, std::string> tags;
    std::map<std::string, std::string> extra;
    std::vector<std::string> fingerprint;
    std::string run_id;
    std::string run_path;
};

static std::mutex event_lock;
static std::mutex breadcrumb_lock;

static SentryEvent sentry_event = {
    .release = nullptr,
    .level = SENTRY_LEVEL_ERROR,
    .dist = nullptr,
    .environment = nullptr,
    .transaction = nullptr,
    .user = std::map<std::string, std::string>(),
    .tags = std::map<std::string, std::string>(),
    .extra = std::map<std::string, std::string>(),
    .fingerprint = std::vector<std::string>()};

static char *BREADCRUMB_CURRENT_FILE =
    BREADCRUMB_FILE_1; /* start off pointing at 1 */
static int breadcrumb_count = 0;

char *sane_strdup(const char *s) {
    if (s) {
        size_t len = strlen(s) + 1;
        char *rv = (char *)malloc(len);
        memcpy(rv, s, len);
        return rv;
    }
    return 0;
}

static const sentry_options_t *sentry_options;

const sentry_options_t *sentry__get_options(void) {
    return sentry_options;
}

static int parse_dsn(char *dsn, SentryDsn *dsn_out) {
    char *ptr, *tmp, *end;

    if (strncmp(dsn, "http://", 7) == 0) {
        ptr = dsn + 7;
        dsn_out->scheme = "http";
    } else if (strncmp(dsn, "https://", 8) == 0) {
        ptr = dsn + 8;
        dsn_out->scheme = "https";
    } else {
        return SENTRY_ERROR_INVALID_URL_SCHEME;
    }

    tmp = strchr(ptr, '?');
    if (tmp) {
        *tmp = '\0';
    }

    end = strchr(ptr, '@');
    if (!end) {
        return 1;
    }
    *end = '\0';
    dsn_out->public_key = ptr;

    tmp = strchr(ptr, ':');
    if (tmp) {
        *tmp = '\0';
        dsn_out->private_key = tmp + 1;
    } else {
        dsn_out->private_key = "";
    }

    ptr = end + 1;
    end = strchr(ptr, '/');
    if (!end) {
        return SENTRY_ERROR_INVALID_URL_MISSING_HOST;
    }
    *end = '\0';
    dsn_out->host = ptr;

    ptr = end + 1;
    end = strrchr(ptr, '/');
    if (end) {
        *end = '\0';
        dsn_out->path = ptr;
        dsn_out->project_id = end + 1;
    } else {
        dsn_out->path = "";
        dsn_out->project_id = ptr;
    }

    return 0;
}

static int minidump_url_from_dsn(char *dsn, std::string &minidump_url_out) {
    /*  Convert DSN to minidump URL i.e:
        From: https://5fd7a6cda8444965bade9ccfd3df9882@sentry.io/1188141
        To:
        https://sentry.io/api/1188141/minidump/?sentry_key=5fd7a6cda8444965bade9ccfd3df9882
    */
    SentryDsn dsn_out;
    auto rv = parse_dsn(dsn, &dsn_out);
    if (rv != 0) {
        return rv;
    }

    minidump_url_out = std::string(dsn_out.scheme) + "://" + dsn_out.host;
    if (dsn_out.path != nullptr && *dsn_out.path) {
        minidump_url_out += std::string("/") + dsn_out.path;
    }
    minidump_url_out += std::string("/api/") + dsn_out.project_id +
                        "/minidump/?sentry_key=" + dsn_out.public_key;
    return 0;
}

static char *to_string_level(sentry_level_t level) {
    /* https://github.com/getsentry/semaphore/blob/331b97bd3c6b7d5ea754aaa75b8e1c4083da86c0/general/src/protocol/types.rs#L513-L519
     */
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

static void serialize(const SentryEvent *event) {
    mpack_writer_t writer;
    /* TODO: cycle event file */
    /* Path must exist otherwise mpack will fail to write. */
    auto dest_path = (event->run_path + SENTRY_EVENT_FILE_NAME).c_str();
    SENTRY_PRINT_DEBUG_ARGS("Serializing to file: %s\n", dest_path);
    mpack_writer_init_filename(&writer, dest_path);
    mpack_start_map(&writer, 10);
    mpack_write_cstr(&writer, "release");
    mpack_write_cstr_or_nil(&writer, event->release);
    mpack_write_cstr(&writer, "level");
    mpack_write_cstr(&writer, to_string_level(event->level));

    mpack_write_cstr(&writer, "user");
    if (!event->user.empty()) {
        mpack_start_map(&writer, event->user.size());
        std::map<std::string, std::string>::const_iterator iter;
        for (iter = event->user.begin(); iter != event->user.end(); ++iter) {
            mpack_write_cstr(&writer, iter->first.c_str());
            mpack_write_cstr_or_nil(&writer, iter->second.c_str());
        }
        mpack_finish_map(&writer);
    } else {
        mpack_write_nil(&writer);
    }

    mpack_write_cstr(&writer, "dist");
    mpack_write_cstr_or_nil(&writer, event->dist);
    mpack_write_cstr(&writer, "environment");
    mpack_write_cstr_or_nil(&writer, event->environment);
    mpack_write_cstr(&writer, "transaction");
    mpack_write_cstr_or_nil(&writer, event->transaction);

    int tag_count = event->tags.size();
    mpack_write_cstr(&writer, "tags");
    mpack_start_map(&writer, tag_count); /* tags */
    if (tag_count > 0) {
        std::map<std::string, std::string>::const_iterator iter;
        for (iter = event->tags.begin(); iter != event->tags.end(); ++iter) {
            mpack_write_cstr(&writer, iter->first.c_str());
            mpack_write_cstr_or_nil(&writer, iter->second.c_str());
        }
    }
    mpack_finish_map(&writer); /* tags */

    int extra_count = event->extra.size();
    mpack_write_cstr(&writer, "extra");
    mpack_start_map(&writer, extra_count); /* extra */
    if (extra_count > 0) {
        std::map<std::string, std::string>::const_iterator iter;
        for (iter = event->extra.begin(); iter != event->extra.end(); ++iter) {
            mpack_write_cstr(&writer, iter->first.c_str());
            mpack_write_cstr_or_nil(&writer, iter->second.c_str());
        }
    }
    mpack_finish_map(&writer); /* extra */

    int fingerprint_count = event->fingerprint.size();
    mpack_write_cstr(&writer, "fingerprint");
    mpack_start_array(&writer, fingerprint_count); /* fingerprint */
    if (fingerprint_count > 0) {
        for (auto part : event->fingerprint) {
            mpack_write_cstr_or_nil(&writer, part.c_str());
        }
    }
    mpack_finish_array(&writer); /* fingerprint */

    mpack_write_cstr(&writer, "sdk");
    mpack_start_map(&writer, 3); /* sdk */
    mpack_write_cstr(&writer, "name");
    mpack_write_cstr(&writer, "sentry.native");
    mpack_write_cstr(&writer, "version");
    mpack_write_cstr(&writer, SENTRY_SDK_VERSION);
    mpack_write_cstr(&writer, "packages");
    mpack_start_array(&writer, 1); /* packages */
    mpack_start_map(&writer, 2);   /* package */
    mpack_write_cstr(&writer, "name");
    mpack_write_cstr(&writer, "github:getsentry/sentrypad");
    mpack_write_cstr(&writer, "version");
    mpack_write_cstr(&writer, SENTRY_SDK_VERSION);
    mpack_finish_map(&writer);   /* package */
    mpack_finish_array(&writer); /* packages */
    mpack_finish_map(&writer);   /* sdk */

    mpack_finish_map(&writer); /* root */

    if (mpack_writer_destroy(&writer) != mpack_ok) {
        SENTRY_PRINT_ERROR("An error occurred encoding the data.\n");
        return;
    }
    /* atomic move on event file */
    /* breadcrumb will send send both files */
}

int sentry_init(const sentry_options_t *options) {
    sentry_options = options;

    if (options->dsn == nullptr) {
        SENTRY_PRINT_ERROR("Not DSN specified. Sentry SDK will be disabled.\n");
        return SENTRY_ERROR_NO_DSN;
    }

    std::string minidump_url;
    auto *dsn = strdup(options->dsn);
    auto err = minidump_url_from_dsn(dsn, minidump_url);
    free(dsn);
    if (err != 0) {
        return err;
    }

    SENTRY_PRINT_DEBUG_ARGS("Initializing with minidump endpoint: %s\n",
                            minidump_url.c_str());

    if (options->environment != nullptr) {
        sentry_event.environment = options->environment;
    }

    if (options->release != nullptr) {
        sentry_event.release = options->release;
    }

    if (options->dist != nullptr) {
        sentry_event.dist = options->dist;
    }

    std::random_device seed;
    std::default_random_engine engine(seed());
    std::uniform_int_distribution<int> uniform_dist(0, INT32_MAX);
    std::time_t result = std::time(nullptr);
    std::stringstream ss;
    ss << result << "-" << uniform_dist(engine);
    sentry_event.run_id = ss.str();

    /* Make sure run dir exists before serializer needs to write to it */
    /* TODO: Write proper x-plat mkdir */
    auto run_path = std::string(options->database_path);
    mkdir(run_path.c_str(), 0700);
    run_path = run_path + "/" + "sentry-runs/";
    mkdir(run_path.c_str(), 0700);
    sentry_event.run_path = run_path + sentry_event.run_id + "/";
    auto rv = mkdir(sentry_event.run_path.c_str(), 0700);
    if (rv != 0 && rv != EEXIST) {
        SENTRY_PRINT_ERROR_ARGS("Failed to create sentry_runs directory '%s'\n",
                                sentry_event.run_path.c_str());
        return rv;
    }

    /* TODO: Reset old runs (i.e: delete old run_paths) */

    std::map<std::string, std::string> attachments =
        std::map<std::string, std::string>();

    attachments.insert(
        std::make_pair(SENTRY_EVENT_FILE_ATTACHMENT_NAME,
                       (sentry_event.run_path + SENTRY_EVENT_FILE_NAME)));
    attachments.insert(
        std::make_pair(SENTRY_BREADCRUMB1_FILE_ATTACHMENT_NAME,
                       (sentry_event.run_path + BREADCRUMB_FILE_1)));
    attachments.insert(
        std::make_pair(SENTRY_BREADCRUMB2_FILE_ATTACHMENT_NAME,
                       (sentry_event.run_path + BREADCRUMB_FILE_2)));

    if (options->attachments) {
        for (int i = 0; i < ATTACHMENTS_MAX; i++) {
            auto attachment = options->attachments[i];
            if (!attachment) break;

            const char *split = strchr(attachment, '=');

            if (split) {
                auto key =
                    std::string(attachment, (size_t)(split - attachment));
                attachments.insert(std::make_pair(key, split + 1));
            } else {
                SENTRY_PRINT_DEBUG_ARGS(
                    "Attachment '%s' didn't match expected format: "
                    "'file=path'\n",
                    attachment);
            }
        }
    }

    err = init(options, minidump_url.c_str(), attachments);

    return 0;
}

void sentry_options_init(sentry_options_t *options) {
}

int serialize_breadcrumb(sentry_breadcrumb_t *breadcrumb,
                         char **data,
                         size_t *size) {
    static mpack_writer_t writer;

    mpack_writer_init_growable(&writer, data, size);
    mpack_start_map(&writer, 5);
    mpack_write_cstr(&writer, "timestamp");
    time_t now;
    time(&now);
    char buf[sizeof "0000-00-00T00:00:00Z"];
    strftime(buf, sizeof buf, "%FT%TZ", gmtime(&now));
    mpack_write_cstr_or_nil(&writer, buf);
    mpack_write_cstr(&writer, "message");
    mpack_write_cstr_or_nil(&writer, breadcrumb->message);
    mpack_write_cstr(&writer, "type");
    mpack_write_cstr_or_nil(&writer, breadcrumb->type);
    mpack_write_cstr(&writer, "category");
    mpack_write_cstr_or_nil(&writer, breadcrumb->category);
    mpack_write_cstr(&writer, "level");
    mpack_write_cstr(&writer, to_string_level(breadcrumb->level));
    mpack_finish_map(&writer);
    if (mpack_writer_destroy(&writer) != mpack_ok) {
        SENTRY_PRINT_ERROR("An error occurred encoding the data.\n");
        return SENTRY_ERROR_BREADCRUMB_SERIALIZATION;
    }

    return 0;
}

int sentry_add_breadcrumb(sentry_breadcrumb_t *breadcrumb) {
    BEGIN_MODIFY_BREADCRUMB;

    if (breadcrumb_count == BREADCRUMB_MAX) {
        /* swap files */
        BREADCRUMB_CURRENT_FILE = BREADCRUMB_CURRENT_FILE == BREADCRUMB_FILE_1
                                      ? BREADCRUMB_FILE_2
                                      : BREADCRUMB_FILE_1;
        breadcrumb_count = 0;
    }

    char *data;
    size_t size;
    auto rv = serialize_breadcrumb(breadcrumb, &data, &size);
    if (rv != 0) {
        return rv;
    }

    auto file = fopen((sentry_event.run_path + BREADCRUMB_CURRENT_FILE).c_str(),
                      breadcrumb_count == 0 ? "w" : "a");

    if (file != NULL) {
        /* consider error handling here */
        EINTR_RETRY(fwrite(data, 1, size, file));
        fclose(file);
    } else {
        SENTRY_PRINT_ERROR_ARGS("Failed to open breadcrumb file %s\n",
                                BREADCRUMB_CURRENT_FILE);
    }

    free(data);

    breadcrumb_count++;
    END_MODIFY_BREADCRUMB;
}

int sentry_set_fingerprint(const char *fingerprint, ...) {
    BEGIN_MODIFY_EVENT;
    va_list va;
    va_start(va, fingerprint);

    if (!fingerprint) {
        sentry_event.fingerprint.clear();
    } else {
        sentry_event.fingerprint.push_back(fingerprint);
        while (1) {
            const char *arg = va_arg(va, const char *);
            if (!arg) {
                break;
            }
            sentry_event.fingerprint.push_back(arg);
        }
    }

    va_end(va);

    END_MODIFY_EVENT;
    return 0;
}

int sentry_remove_fingerprint(void) {
    return sentry_set_fingerprint(NULL);
}

int sentry_set_level(enum sentry_level_t level) {
    BEGIN_MODIFY_EVENT;
    sentry_event.level = level;
    END_MODIFY_EVENT;
    return 0;
}

int sentry_set_transaction(const char *transaction) {
    BEGIN_MODIFY_EVENT;
    sentry_event.transaction = transaction;
    END_MODIFY_EVENT;
    return 0;
}

int sentry_remove_transaction() {
    sentry_set_transaction(nullptr);
}

int sentry_set_user(const sentry_user_t *user) {
    BEGIN_MODIFY_EVENT;
    sentry_user_t *new_user = (sentry_user_t *)malloc(sizeof(sentry_user_t));
    sentry_event.user.clear();

    if (user->id) {
        sentry_event.user.insert(std::make_pair("id", user->id));
    }
    if (user->username) {
        sentry_event.user.insert(std::make_pair("username", user->username));
    }
    if (user->email) {
        sentry_event.user.insert(std::make_pair("email", user->email));
    }
    if (user->ip_address) {
        sentry_event.user.insert(
            std::make_pair("ip_address", user->ip_address));
    }

    END_MODIFY_EVENT;
    return 0;
}

int sentry_remove_user() {
    int rv = sentry_set_user(nullptr);
    return rv;
}

int sentry_set_tag(const char *key, const char *value) {
    BEGIN_MODIFY_EVENT;
    sentry_event.tags[key] = value;
    END_MODIFY_EVENT;
    return 0;
}

int sentry_remove_tag(const char *key) {
    BEGIN_MODIFY_EVENT;
    sentry_event.tags.erase(key);
    END_MODIFY_EVENT;
    return 0;
}

int sentry_set_extra(const char *key, const char *value) {
    BEGIN_MODIFY_EVENT;
    sentry_event.extra[key] = value;
    END_MODIFY_EVENT;
    return 0;
}

int sentry_remove_extra(const char *key) {
    BEGIN_MODIFY_EVENT;
    sentry_event.extra.erase(key);
    END_MODIFY_EVENT;
    return 0;
}

int sentry_set_release(const char *release) {
    BEGIN_MODIFY_EVENT;
    sentry_event.release = release;
    END_MODIFY_EVENT;
    return 0;
}

int sentry_remove_release() {
    return sentry_set_release(nullptr);
}

void sentry_user_clear(sentry_user_t *user) {
    user->email = nullptr;
    user->id = nullptr;
    user->ip_address = nullptr;
    user->username = nullptr;
}