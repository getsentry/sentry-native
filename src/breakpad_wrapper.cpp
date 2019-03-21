#if defined(SENTRY_BREAKPAD)
#include <stdio.h>
#include <atomic>
#include <map>
#include <string>
#if defined(__APPLE__)
#include "client/mac/handler/exception_handler.h"
#elif defined(__linux__)
#include "client/linux/handler/exception_handler.h"
#endif
#include "macros.hpp"
#include "sentry.h"

namespace sentry {
namespace breakpad {

using namespace google_breakpad;

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
        SENTRY_PRINT_ERROR("Crashpad minidump creation failed.");
    }
    return succeeded;
}
#endif

breakpad::ExceptionHandler *handler;

int init(const sentry_options_t *options,
         const char *minidump_url,
         std::map<std::string, std::string> attachments) {
    SENTRY_PRINT_DEBUG_ARGS("Initializing Breakpad with directory: %s\n",
                            options->database_path);

#if defined(__APPLE__)
    handler =
        new breakpad::ExceptionHandler(options->database_path, 0, callback, 0, true, 0);
#elif defined(__linux__)
    MinidumpDescriptor descriptor(options->database_path);
    handler = new ExceptionHandler(descriptor,
                                   /* filter */ nullptr, callback,
                                   /* context */ nullptr,
                                   /* install handler */ true,
                                   /* server FD */ -1);
#endif
    return 0;
}
} /* namespace breakpad */
} /* namespace sentry */
#endif
