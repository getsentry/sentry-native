#if defined(SENTRY_BREAKPAD)
#include <stdio.h>
#include <atomic>
#include <map>
#include <string>
#include <vector>
#include "client/mac/handler/exception_handler.h"
#include "common/linux/http_upload.h"

#include "macros.hpp"
#include "sentry.h"
namespace sentry {
namespace breakpad {

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

bool UploadMinidump(std::string &path) {
    // Add additional arguments for Sentry
    std::map<string, string> parameters;

    std::map<string, string> files;
    files["upload_file_minidump"] = path;

    // return google_breakpad::HTTPUpload::SendRequest(
    //     "https://sentry.io/api/1/minidump/"
    //     "?sentry_key=16427b2f210046b585ee51fd8a1ac54f",
    //     parameters, files,
    //     /* proxy */ "",
    //     /* proxy password */ "",
    //     /* certificate */ "",
    //     /* response body */ nullptr,
    //     /* response code */ nullptr,
    //     /* error */ nullptr);
    return true;
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
