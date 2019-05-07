#if defined(SENTRY_BREAKPAD)
#if defined(__linux__) || defined(__APPLE__)
#include <dirent.h>
#endif
#include <sys/errno.h>
#include <sys/stat.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#if defined(__APPLE__)
#include "client/mac/handler/exception_handler.h"
#elif defined(__linux__)
#include "client/linux/handler/exception_handler.h"
#include "common/linux/http_upload.h"
#endif
#include "macros.hpp"
#include "sentry.h"
#include "vendor/mpack.h"

namespace sentry {

const SentryInternalOptions *sentry_internal_options;

#if defined(__APPLE__)
bool callback(const char *dump_dir,
              const char *minidump_id,
              void *context,
              bool succeeded) {
    if (succeeded) {
        SENTRY_PRINT_DEBUG_ARGS("Breakpad Minidump created at: %s/%s", dump_dir,
                                minidump_id);
    }

    return succeeded;
}
int upload(std::string minidump_url,
           std::map<std::string, std::string> attachments);
#elif defined(__linux__)

int upload(std::string minidump_url,
           std::map<std::string, std::string> attachments) {
    std::map<std::string, std::string> files(attachments);

    // Add additional arguments for Sentry
    std::map<std::string, std::string> parameters;

    SENTRY_PRINT_DEBUG_ARGS("Uploading %lu files:", files.size());
    std::map<std::string, std::string>::const_iterator iter;
    for (iter = files.begin(); iter != files.end(); ++iter) {
        SENTRY_PRINT_DEBUG_ARGS("\t%s=%s", iter->first.c_str(),
                                iter->second.c_str());
    }

    return HTTPUpload::SendRequest(minidump_url, parameters, files,
                                   /* proxy */ "",
                                   /* proxy password */ "",
                                   /* certificate */ "",
                                   /* response body */ nullptr,
                                   /* response code */ nullptr,
                                   /* error */ nullptr);
}

bool callback(const MinidumpDescriptor &descriptor,
              void *context,
              bool succeeded) {
    if (succeeded) {
        SENTRY_PRINT_DEBUG_ARGS("Breakpad Minidump created at: %s",
                                descriptor.path());
    }

    return succeeded;
}

#endif

struct SentryRunInfo {
    std::string minidump_url;
    std::map<std::string, std::string> attachments;
};

google_breakpad::ExceptionHandler *handler;
static const char *SENTRY_BREAKPAD_RUN_INFO_FILE_NAME = "sentry-db.mp";

int serialize_run_info(const char *dest_path, const SentryRunInfo *info) {
    std::string file_path =
        (std::string(dest_path) + SENTRY_BREAKPAD_RUN_INFO_FILE_NAME);
    SENTRY_PRINT_DEBUG_ARGS("Writing database files: %s", file_path.c_str());

    mpack_writer_t writer;
    mpack_writer_init_filename(&writer, file_path.c_str());

    mpack_write_cstr(&writer, info->minidump_url.c_str());
    SENTRY_PRINT_DEBUG_ARGS("Writing minidump_url: %s",
                            info->minidump_url.c_str());

    size_t attachments_size = info->attachments.size();
    mpack_start_map(&writer, attachments_size); /* attachments */
    if (attachments_size > 0) {
        std::map<std::string, std::string>::const_iterator iter;
        for (iter = info->attachments.begin(); iter != info->attachments.end();
             ++iter) {
            mpack_write_cstr(&writer, iter->first.c_str());
            mpack_write_cstr(&writer, iter->second.c_str());
            SENTRY_PRINT_DEBUG_ARGS("Attachment: %s=%s", iter->first.c_str(),
                                    iter->second.c_str());
        }
    }
    mpack_finish_map(&writer); /* attachments */
    if (mpack_writer_destroy(&writer) != mpack_ok) {
        SENTRY_PRINT_ERROR(
            "An error occurred while writing sentry run information. Error "
            "encoding the data.");
        return SENTRY_ERROR_SERIALIZING_SENTRY_RUN_INFO;
    }

    return 0;
}

int deserialize_run_info(const char *path, SentryRunInfo *run_info) {
    std::string file_path =
        std::string(path) + SENTRY_BREAKPAD_RUN_INFO_FILE_NAME;
    SENTRY_PRINT_DEBUG_ARGS("Reading run information from path: %s",
                            file_path.c_str());

    mpack_reader_t reader;
    mpack_reader_init_filename(&reader, file_path.c_str());

    char minidump_url[MINIDUMP_URL_MAX_LENGTH];
    mpack_expect_cstr(&reader, minidump_url, sizeof(minidump_url));
    SENTRY_PRINT_DEBUG_ARGS("Minidump URL: %s", minidump_url);
    run_info->minidump_url = minidump_url;

    size_t count = mpack_expect_map_max(&reader, ATTACHMENTS_MAX);

    for (size_t i = count; i > 0 && mpack_reader_error(&reader) == mpack_ok;
         --i) {
        char key[ATTACHMENTS_KEY_LENGTH_MAX];
        mpack_expect_cstr(&reader, key, sizeof(key));
        char value[ATTACHMENTS_PATH_LENGTH_MAX];
        mpack_expect_cstr(&reader, value, sizeof(value));
        SENTRY_PRINT_DEBUG_ARGS("Attachment: %s=%s", key, value);

        run_info->attachments.insert(std::make_pair(key, value));
    }
    mpack_done_map(&reader);

    mpack_error_t error = mpack_reader_destroy(&reader);
    if (error != mpack_ok) {
        SENTRY_PRINT_DEBUG_ARGS("Failed reading msgpack: %d", error);
        return SENTRY_ERROR_DESERIALIZING_SENTRY_RUN_INFO;
    }

    return 0;
}

bool has_ending(const std::string &fullString, const std::string &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare(fullString.length() - ending.length(),
                                        ending.length(), ending));
    } else {
        return false;
    }
}

int create_directory(const char *path) {
#ifdef _WIN32
    int rv = SHCreateDirectoryExA(NULL, path, NULL);
    if (rv == ERROR_SUCCESS || rv == ERROR_ALREADY_EXISTS ||
        ERROR_FILE_EXISTS) {
        return 0;
    } else {
        SENTRY_PRINT_ERROR_ARGS("Failed to create directory '%s'", path);
        return rv;
    }
#else
#define _TRY_MAKE_DIR                                                         \
    do {                                                                      \
        rv = mkdir(p, 0700);                                                  \
        if (rv != 0 && errno != EEXIST) {                                     \
            SENTRY_PRINT_ERROR_ARGS("Failed to create directory '%s'", path); \
            goto done;                                                        \
        }                                                                     \
    } while (0)

    int rv = 0;
    char *p = strdup(path);
    char *ptr;
    for (ptr = p; *ptr; ptr++) {
        if (*ptr == '/') {
            *ptr = 0;
            _TRY_MAKE_DIR;
            *ptr = '/';
        }
    }
    _TRY_MAKE_DIR;
#undef _TRY_MAKE_DIR
done:
    free(p);
    return 0;
#endif
}

int remove_directory(const char *path) {
#ifdef _WIN32
    str::std::string p(path);
    p += '\0';
    SHFILEOPSTRUCTA fos = {0};
    fos.wFunc = FO_DELETE;
    fos.pFrom = p.c_str();
    fos.fFlags = FOF_NO_UI;
    return SHFileOperation(&fos);
#else
    DIR *dp;
    struct dirent *dirp;

    if ((dp = opendir(path)) == nullptr) {
        return 1;
    }

    while ((dirp = readdir(dp)) != nullptr) {
        if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0) {
            continue;
        }
        std::string p(path);
        if (p[p.length() - 1] != '/') {
            p += '/';
        }
        p += dirp->d_name;
        if (remove(p.c_str()) != 0) {
            return 1;
        }
    }

    closedir(dp);

    remove(path);
    return 0;
#endif
}

int upload_last_runs(const char *database_path) {
    // TODO: Take the runs_path and pending path as arg, single place to build
    // runs_path
    DIR *dp;
    struct dirent *dirp;
    std::string runs = std::string(database_path) + "/sentry-runs/";
    std::string pending = std::string(database_path) + "/sentry-pending/";

    // Make sure pending exists way before getting here
    int rv = create_directory(pending.c_str());
    if (rv != 0) {
        return rv;
    }

    if ((dp = opendir(runs.c_str())) == nullptr) {
        SENTRY_PRINT_ERROR_ARGS("Failed to open database directory. %s",
                                database_path);
        return SENTRY_ERROR_FAILED_READING_DATABASE_DIRECTORY;
    }

    // Look through run directory for pending uploads
    while ((dirp = readdir(dp)) != nullptr) {
        if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0) {
            continue;
        }

        std::string from = runs + dirp->d_name;
        SENTRY_PRINT_DEBUG_ARGS("Looking for runs with minidump files at: %s",
                                dirp->d_name);

        // Make sure there's a .dmp inside otherwise skip (might be in use
        // by another instance)
        DIR *inner_dp;
        if ((inner_dp = opendir(from.c_str())) == nullptr) {
            continue;
        }

        struct dirent *filep;
        while ((filep = readdir(inner_dp)) != nullptr) {
            if (strcmp(dirp->d_name, ".") == 0 ||
                strcmp(dirp->d_name, "..") == 0) {
                continue;
            }

            if (!has_ending(filep->d_name, MINIDUMP_FILE_EXTENSION)) {
                continue;
            }
            SENTRY_PRINT_DEBUG_ARGS("Found minidump file: %s", filep->d_name);

            std::string run_dir = runs + dirp->d_name + "/";
            std::string minidump_from = run_dir + filep->d_name;
            std::string pending_run = pending + dirp->d_name;
            // Make sure run directory exists
            SENTRY_PRINT_DEBUG_ARGS("Making pending folder: %s",
                                    pending_run.c_str());
            int rv = create_directory(pending_run.c_str());
            if (rv != 0) {
                continue;
            }
            std::string minidump_to = pending_run + "/" + filep->d_name;
            SENTRY_PRINT_DEBUG_ARGS("Moving minidump from: %s to %s",
                                    minidump_from.c_str(), minidump_to.c_str());

            int err = rename(minidump_from.c_str(), minidump_to.c_str());
            if (err != 0) {
                SENTRY_PRINT_ERROR("Failed to move minidump.");
                continue;
            }

            SentryRunInfo run_info;
            rv = deserialize_run_info(run_dir.c_str(), &run_info);
            if (rv != 0) {
                continue;
            }
            SENTRY_PRINT_DEBUG_ARGS("Uploading run with minidump_url %s",
                                    run_info.minidump_url.c_str());

            // TODO: Copy attachments first
            run_info.attachments.insert(
                make_pair("upload_file_minidump", minidump_to));

            rv = upload(run_info.minidump_url, run_info.attachments);
            if (rv == 0) {
                SENTRY_PRINT_DEBUG(
                    "Crash upload successful! Removing run and pending "
                    "directories.");
                std::string event =
                    run_dir + SENTRY_BREAKPAD_RUN_INFO_FILE_NAME;
                int rv = remove_directory(run_dir.c_str());
                if (rv != 0) {
                    SENTRY_PRINT_ERROR_ARGS("Failed to remove dir: %s",
                                            run_dir.c_str());
                }
                rv = remove_directory(pending_run.c_str());
                if (rv != 0) {
                    SENTRY_PRINT_ERROR_ARGS("Failed to remove dir: %s",
                                            run_dir.c_str());
                }
            } else {
                if (rv == 100) {
                    rename(minidump_to.c_str(), minidump_from.c_str());
                } else {
                    SENTRY_PRINT_ERROR_ARGS("Failed to upload with code: %d",
                                            rv);
                }
            }
        }

        closedir(inner_dp);
    }
    closedir(dp);

    return 0;
}

int init(const SentryInternalOptions *sentry_internal_options) {
    SENTRY_PRINT_DEBUG_ARGS("Initializing Breakpad with directory: %s",
                            sentry_internal_options->run_path.c_str());
    sentry_internal_options = sentry_internal_options;

#if defined(__APPLE__)
    handler = new google_breakpad::ExceptionHandler(
        sentry_internal_options->run_path, 0, callback, 0, true, 0);
#elif defined(__linux__)

    MinidumpDescriptor descriptor(sentry_internal_options->run_path);
    handler =
        new google_breakpad::ExceptionHandler(descriptor,
                                              /* filter */ nullptr, callback,
                                              /* context */ nullptr,
                                              /* install handler */ true,
                                              /* server FD */ -1);
#endif

    /* Serialize DSN, attachment name and path for the case of a crash,
    next run will be able to upload it. */
    SentryRunInfo run_info{
        .minidump_url = sentry_internal_options->minidump_url,
        .attachments = sentry_internal_options->attachments};

    int rv = serialize_run_info(sentry_internal_options->run_path.c_str(),
                                &run_info);

    rv = upload_last_runs(sentry_internal_options->options.database_path);
    if (rv != 0) {
        return rv;
    }
    return 0;
}
} /* namespace sentry */
#endif
