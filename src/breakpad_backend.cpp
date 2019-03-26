#if defined(SENTRY_BREAKPAD)
#if defined(__linux__)
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

using namespace google_breakpad;
using namespace std;

const SentryInternalOptions *sentry_internal_options;

#if defined(__APPLE__)
bool callback(const char *dump_dir,
              const char *minidump_id,
              void *context,
              bool succeeded) {
    if (succeeded) {
        printf("%s/%s.dmp\n", dump_dir, minidump_id);
    } else {
        printf("ERROR creating minidump\n");
    }

    return succeeded;
}
#elif defined(__linux__)

int upload(string minidump_url, map<string, string> attachments) {
    std::map<string, string> files(attachments);

    // Add additional arguments for Sentry
    std::map<string, string> parameters;

    SENTRY_PRINT_DEBUG_ARGS("Uploading %lu files:\n", files.size());
    std::map<std::string, std::string>::const_iterator iter;
    for (iter = files.begin(); iter != files.end(); ++iter) {
        SENTRY_PRINT_DEBUG_ARGS("\t%s=", iter->first.c_str());
        SENTRY_PRINT_DEBUG_ARGS("%s\n", iter->second.c_str());
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
    // if succeeded is true, descriptor.path() contains a path
    // to the minidump file. Context is the context passed to
    // the exception handler's constructor.
    if (succeeded) {
        SENTRY_PRINT_DEBUG_ARGS("Breakpad Minidump created at: %s\n",
                                descriptor.path());
    } else {
        SENTRY_PRINT_ERROR("Breakpad minidump creation failed.");
    }

    return succeeded;
}

#endif

struct SentryRunInfo {
    std::string minidump_url;
    map<string, string> attachments;
};

ExceptionHandler *handler;
static const char *SENTRY_BREAKPAD_RUN_INFO_FILE_NAME = "sentry-db.mp";

int serialize_run_info(const char *dest_path, const SentryRunInfo *info) {
    auto file_path =
        (string(dest_path) + SENTRY_BREAKPAD_RUN_INFO_FILE_NAME).c_str();
    SENTRY_PRINT_DEBUG_ARGS("Writing database files: %s\n", file_path);

    mpack_writer_t writer;
    mpack_writer_init_filename(&writer, file_path);

    mpack_write_cstr(&writer, info->minidump_url.c_str());
    SENTRY_PRINT_DEBUG_ARGS("Writing minidump_url: %s\n",
                            info->minidump_url.c_str());

    auto attachments_size = info->attachments.size();
    mpack_start_map(&writer, attachments_size); /* attachments */
    if (attachments_size > 0) {
        std::map<std::string, std::string>::const_iterator iter;
        for (iter = info->attachments.begin(); iter != info->attachments.end();
             ++iter) {
            mpack_write_cstr(&writer, iter->first.c_str());
            mpack_write_cstr(&writer, iter->second.c_str());
            SENTRY_PRINT_DEBUG_ARGS("Attachment Key: %s", iter->first.c_str());
            SENTRY_PRINT_DEBUG_ARGS(" Value: %s\n", iter->second.c_str());
        }
    }
    mpack_finish_map(&writer); /* attachments */
    if (mpack_writer_destroy(&writer) != mpack_ok) {
        SENTRY_PRINT_ERROR(
            "An error occurred while writing sentry run information. Error "
            "encoding the data.\n");
        return SENTRY_ERROR_SERIALIZING_SENTRY_RUN_INFO;
    }

    return 0;
}

int deserialize_run_info(const char *path, SentryRunInfo *run_info) {
    auto file_path =
        (string(path) + SENTRY_BREAKPAD_RUN_INFO_FILE_NAME).c_str();
    SENTRY_PRINT_DEBUG_ARGS("Reading run information from path: %s\n",
                            file_path);

    mpack_reader_t reader;
    mpack_reader_init_filename(&reader, file_path);

    char minidump_url[MINIDUMP_URL_MAX_LENGTH];
    mpack_expect_cstr(&reader, minidump_url, sizeof(minidump_url));
    SENTRY_PRINT_DEBUG_ARGS("Minidump URL: %s\n", minidump_url);
    run_info->minidump_url = minidump_url;

    size_t count = mpack_expect_map_max(&reader, ATTACHMENTS_MAX);

    SENTRY_PRINT_DEBUG_ARGS("# of attachments: %d\n", count);
    for (size_t i = count; i > 0 && mpack_reader_error(&reader) == mpack_ok;
         --i) {
        char key[ATTACHMENTS_KEY_LENGTH_MAX];
        mpack_expect_cstr(&reader, key, sizeof(key));
        char value[ATTACHMENTS_PATH_LENGTH_MAX];
        mpack_expect_cstr(&reader, value, sizeof(value));
        SENTRY_PRINT_DEBUG_ARGS("Attachment key: %s", key);
        SENTRY_PRINT_DEBUG_ARGS(" Value: %s\n", value);

        run_info->attachments.insert(std::make_pair(key, value));
    }
    mpack_done_map(&reader);

    mpack_error_t error = mpack_reader_destroy(&reader);
    if (error != mpack_ok) {
        SENTRY_PRINT_DEBUG_ARGS("Failed reading msgpack: %d\n", error);
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
    // TODO: x-plat create dir
    auto rv = mkdir(path, 0700);
    if (rv != 0) {
        if (errno == EEXIST) {
            SENTRY_PRINT_DEBUG_ARGS("Directory '%s' exists.\n", path);
            return 0;
        } else {
            SENTRY_PRINT_ERROR_ARGS("Failed to create directory '%s'\n", path);
        }
    } else {
        SENTRY_PRINT_DEBUG_ARGS("Created directory: %s\n", path);
    }
    return rv;
}

int upload_last_runs(const char *database_path) {
    // TODO: Take the runs_path and pending path as arg, single place to build
    // runs_path
    DIR *dp;
    struct dirent *dirp;
    auto runs = string(database_path) + "/sentry-runs/";
    auto pending = string(database_path) + "/sentry-pending/";

    // Make sure pending exists way before getting here
    auto rv = create_directory(pending.c_str());
    if (rv != 0) {
        return rv;
    }

    if ((dp = opendir(runs.c_str())) == nullptr) {
        SENTRY_PRINT_ERROR_ARGS("Failed to open database directory. %s\n",
                                database_path);
        return SENTRY_ERROR_FAILED_READING_DATABASE_DIRECTORY;
    }

    // Look through run directory for pending uploads
    while ((dirp = readdir(dp)) != nullptr) {
        if (string(dirp->d_name) == "." || string(dirp->d_name) == "..") {
            continue;
        }

        auto from = runs + dirp->d_name;
        SENTRY_PRINT_DEBUG_ARGS("Looking for runs with minidump files at: %s\n",
                                dirp->d_name);

        // Make sure there's a .dmp inside otherwise skip (might be in use
        // by another instance)
        DIR *inner_dp;
        if ((inner_dp = opendir(from.c_str())) == nullptr) {
            SENTRY_PRINT_DEBUG("Not a dir?\n");
            continue;
        }

        struct dirent *filep;
        while ((filep = readdir(inner_dp)) != nullptr) {
            if (string(filep->d_name) == "." || string(filep->d_name) == "..") {
                continue;
            }

            auto minidump_extension = MINIDUMP_FILE_EXTENSION;
            if (!has_ending(filep->d_name, minidump_extension)) {
                SENTRY_PRINT_DEBUG_ARGS("Skipping non minidump file: %s\n",
                                        filep->d_name);
                continue;
            }
            SENTRY_PRINT_DEBUG_ARGS("Found minidump file: %s\n", filep->d_name);

            auto run_dir = runs + dirp->d_name + "/";
            auto minidump_from = run_dir + filep->d_name;
            auto pending_run = pending + dirp->d_name;
            // Make sure run directory exists
            auto rv = create_directory(pending_run.c_str());
            if (rv != 0) {
                continue;
            }
            auto minidump_to = pending_run + "/" + filep->d_name;
            SENTRY_PRINT_DEBUG_ARGS("Moving minidump from: %s",
                                    minidump_from.c_str());
            SENTRY_PRINT_DEBUG_ARGS(" to: %s\n", minidump_to.c_str());

            auto err = rename(minidump_from.c_str(), minidump_to.c_str());
            if (err != 0) {
                SENTRY_PRINT_ERROR("Failed to move minidump.\n");
                continue;
            }

            SentryRunInfo run_info;
            rv = deserialize_run_info(run_dir.c_str(), &run_info);
            if (rv != 0) {
                continue;
            }
            SENTRY_PRINT_DEBUG_ARGS("Uploading run with minidump_url %s\n",
                                    run_info.minidump_url.c_str());

            // TODO: Copy attachments first
            run_info.attachments.insert(
                make_pair("upload_file_minidump", minidump_to));

            rv = upload(run_info.minidump_url, run_info.attachments);
            if (rv == 0) {
                SENTRY_PRINT_DEBUG(
                    "Crash upload successful! Removing run and pending "
                    "directories.\n");
                // remove files
                remove(run_dir.c_str());
                remove(pending_run.c_str());
            } else {
                if (rv == 100) {  // TODO handle client offline/retry
                    // Move the minidump back
                } else {
                    SENTRY_PRINT_ERROR_ARGS("Failed to upload with code: %d\n",
                                            rv);
                }
            }
        }

        closedir(inner_dp);
    }
    closedir(dp);

    return 0;
}  // namespace sentry

int init(const SentryInternalOptions *sentry_internal_options) {
    SENTRY_PRINT_DEBUG_ARGS("Initializing Breakpad with directory: %s\n",
                            sentry_internal_options->run_path.c_str());
    sentry_internal_options = sentry_internal_options;

#if defined(__APPLE__)
    handler = new ExceptionHandler(sentry_internal_options->run_path, 0,
                                   callback, 0, true, 0);
#elif defined(__linux__)

    MinidumpDescriptor descriptor(sentry_internal_options->run_path);
    handler = new ExceptionHandler(descriptor,
                                   /* filter */ nullptr, callback,
                                   /* context */ nullptr,
                                   /* install handler */ true,
                                   /* server FD */ -1);
#endif

    /* Serialize DSN, attachment name and path for the case of a crash,
    next run will be able to upload it. */
    auto run_info =
        SentryRunInfo{.minidump_url = sentry_internal_options->minidump_url,
                      .attachments = sentry_internal_options->attachments};

    auto rv = serialize_run_info(sentry_internal_options->run_path.c_str(),
                                 &run_info);

    rv = upload_last_runs(sentry_internal_options->options.database_path);
    if (rv != 0) {
        return rv;
    }
    return 0;
}
} /* namespace sentry */
#endif
