#if defined(SENTRY_BREAKPAD)
#include <stdio.h>
#include <atomic>
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

namespace sentry {

using namespace google_breakpad;

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

bool upload_minidump(const char *path) {
    // Add additional arguments for Sentry
    std::map<string, string> parameters;

    std::map<string, string> files(sentry_internal_options->attachments);
    files["upload_file_minidump"] = path;

    SENTRY_PRINT_DEBUG_ARGS("Uploading minidump: %s\n", path);
    SENTRY_PRINT_DEBUG_ARGS("Total attachments: %d\n", files.size());

    return HTTPUpload::SendRequest(sentry_internal_options->minidump_url,
                                   parameters, files,
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

ExceptionHandler *handler;

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
    // TODO: Find pending crahes and call:
    // upload_minidump(descriptor.path());

    return 0;
}
} /* namespace sentry */
#endif
