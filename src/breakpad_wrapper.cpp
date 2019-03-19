#if defined(SENTRY_BREAKPAD)
#include <stdio.h>
#include <atomic>
#include <map>
#include <string>
#include <vector>
#include "client/mac/handler/exception_handler.h"

#include "macros.hpp"
#include "sentry.h"

namespace sentry {
namespace breakpad {

using namespace google_breakpad;

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

int init(const sentry_options_t *options,
         const char *minidump_url,
         std::map<std::string, std::string> attachments) {
    google_breakpad::ExceptionHandler eh(options->database_path, 0, callback, 0,
                                         true, 0);
    return 0;
}
} /* namespace breakpad */
} /* namespace sentry */
#endif
