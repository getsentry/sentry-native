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

// bool upload_minidump(const char *run_dir) {
//     string dir = string(run_dir);
//     vector<string> files = vector<string>();

//     getdir(dir, files);

//     std::map<string, string>
//     attachments(sentry_internal_options->attachments); for (unsigned int i =
//     0; i < files.size(); i++) {
//         attachments["upload_file_minidump"] = files[i];
//     }

//     // Add additional arguments for Sentry
//     std::map<string, string> parameters;

//     SENTRY_PRINT_DEBUG_ARGS("Uploading minidump: %s\n", path);
//     SENTRY_PRINT_DEBUG_ARGS("Total attachments: %lu\n", attachments.size());

//     return HTTPUpload::SendRequest(sentry_internal_options->minidump_url,
//                                    parameters, attachments,
//                                    /* proxy */ "",
//                                    /* proxy password */ "",
//                                    /* certificate */ "",
//                                    /* response body */ nullptr,
//                                    /* response code */ nullptr,
//                                    /* error */ nullptr);
// }

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

ExceptionHandler *handler;
static const char *SENTRY_BREAKPAD_ATTACHMENTS_FILE_NAME = "attachments.mp";

int serialize_attachments(const char *dest_path,
                          map<string, string> attachments) {
    auto file_path =
        (string(dest_path) + SENTRY_BREAKPAD_ATTACHMENTS_FILE_NAME).c_str();
    SENTRY_PRINT_DEBUG_ARGS("Writing attachment files: %s\n", file_path);

    mpack_writer_t writer;
    mpack_writer_init_filename(&writer, file_path);
    mpack_write_cstr(&writer, "attachments");
    auto attachments_size = attachments.size();
    mpack_start_map(&writer, attachments_size); /* attachments */
    if (attachments_size > 0) {
        std::map<std::string, std::string>::const_iterator iter;
        for (iter = attachments.begin(); iter != attachments.end(); ++iter) {
            mpack_write_cstr(&writer, iter->first.c_str());
            mpack_write_cstr(&writer, iter->second.c_str());
            SENTRY_PRINT_DEBUG_ARGS("Attachment Key: %s", iter->first.c_str());
            SENTRY_PRINT_DEBUG_ARGS(" Value: %s\n", iter->second.c_str());
        }
    }
    mpack_finish_map(&writer); /* attachments */
    if (mpack_writer_destroy(&writer) != mpack_ok) {
        SENTRY_PRINT_ERROR(
            "An error occurred while writing attachment. Error encoding the "
            "data.\n");
        return -1;
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

bool contain_minidump_in_path(DIR *dp) {
    bool contains = false;
    struct dirent *filep;
    while ((filep = readdir(dp)) != nullptr) {
        if (string(filep->d_name) == "." || string(filep->d_name) == "..") {
            continue;
        }

        // TODO: Const
        auto minidump_extension = ".dmp";
        if (!has_ending(filep->d_name, minidump_extension)) {
            SENTRY_PRINT_DEBUG_ARGS("Skipping non minidump file: %s\n",
                                    filep->d_name);
            continue;
        }
        contains = true;
        SENTRY_PRINT_DEBUG_ARGS("Found minidump file: %s\n", filep->d_name);
        break;
    }
    return contains;
}

int upload_last_runs(const char *database_path) {
    // TODO: Take the runs_path and pending path as arg, single place to build
    // runs_path
    DIR *dp;
    struct dirent *dirp;
    auto runs = string(database_path) + "/sentry-runs/";
    auto pending = string(database_path) + "/sentry-pending/";

    // TODO: Make sure pending exists way before getting here
    auto rv = mkdir(pending.c_str(), 0700);
    if (rv != 0 && errno != EEXIST) {
        SENTRY_PRINT_ERROR_ARGS(
            "Failed to create sentry_pending directory '%s'\n",
            pending.c_str());
        return rv;
    }
    if ((dp = opendir(runs.c_str())) == nullptr) {
        SENTRY_PRINT_ERROR_ARGS("Failed to open database directory. %s\n",
                                database_path);
        // TODO: Error code const
        return -2;
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

        if (!contain_minidump_in_path(inner_dp)) {
            SENTRY_PRINT_DEBUG_ARGS(
                "Directory: %s doesn't have a minidump. Might be in use or was "
                "a successful run.\n",
                dirp->d_name);
            continue;
        }
        closedir(inner_dp);

        auto to = pending + dirp->d_name;
        SENTRY_PRINT_DEBUG_ARGS("Moving pending run from: %s", from.c_str());
        SENTRY_PRINT_DEBUG_ARGS(" to: %s\n", to.c_str());

        auto err = rename(from.c_str(), to.c_str());

        if (err != 0) {
            SENTRY_PRINT_ERROR("Failed to move runs directory to pending");
        }
    }
    closedir(dp);

    // move those to sentry_pending

    // upload crash
    // read attachments.mp
    // copy files to working dir
    // remove attachments.mp
    // make request

    return 0;
}

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

    /* Serialize attachment name and path for the case of a crash,
    next run be able to find it and name it correctly */
    auto rv = serialize_attachments(sentry_internal_options->run_path.c_str(),
                                    sentry_internal_options->attachments);
    if (rv != 0) {
        return rv;
    }

    rv = upload_last_runs(sentry_internal_options->options.database_path);
    if (rv != 0) {
        return rv;
    }
    return 0;
}
} /* namespace sentry */
#endif
