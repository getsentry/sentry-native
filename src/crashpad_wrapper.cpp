#if defined(SENTRY_CRASHPAD)
#include <stdio.h>
#include <atomic>
#include <map>
#include <string>
#include <vector>

#include "client/crash_report_database.h"
#include "client/crashpad_client.h"
#include "client/crashpad_info.h"
#include "client/settings.h"
#include "macros.hpp"
#include "sentry.h"

using namespace crashpad;

namespace sentry {
namespace crashpad {
SimpleStringDictionary simple_annotations;

int init(const sentry_options_t *options,
         const char *minidump_url,
         std::map<std::string, std::string> attachments) {
    if (minidump_url == nullptr) {
        return SENTRY_ERROR_NO_MINIDUMP_URL;
    }
    /* Cache directory that will store crashpad information and minidumps */
    base::FilePath database(options->database_path);
    /* Path to the out-of-process handler executable */
    base::FilePath handler(options->handler_path);
    /* URL used to submit minidumps to */
    std::string url(minidump_url);
    /* Optional annotations passed via --annotations to the handler */
    std::map<std::string, std::string> annotations;

    std::map<std::string, base::FilePath> fileAttachments =
        std::map<std::string, base::FilePath>();

    std::map<std::string, std::string>::const_iterator iter;
    for (iter = attachments.begin(); iter != attachments.end(); ++iter) {
        fileAttachments.insert(
            std::make_pair(iter->first, base::FilePath(iter->second)));
    }

    /* Optional arguments to pass to the handler */
    std::vector<std::string> arguments;
    arguments.push_back("--no-rate-limit");

    CrashpadClient client;
    bool success = client.StartHandlerWithAttachments(
        handler, database, database, url, annotations, fileAttachments,
        arguments,
        /* restartable */ true,
        /* asynchronous_start */ false);

    if (success) {
        SENTRY_PRINT_DEBUG("Started client handler.\n");
    } else {
        SENTRY_PRINT_ERROR("Failed to start client handler.\n");
    }

    if (!success) {
        return SENTRY_ERROR_HANDLER_STARTUP_FAIL;
    }

    std::unique_ptr<CrashReportDatabase> db =
        CrashReportDatabase::Initialize(database);

    if (db != nullptr && db->GetSettings() != nullptr) {
        db->GetSettings()->SetUploadsEnabled(true);
    }

    /* Ensure that the simple annotations dictionary is set in the client. */
    CrashpadInfo *crashpad_info = CrashpadInfo::GetCrashpadInfo();
    crashpad_info->set_simple_annotations(&simple_annotations);

    return 0;
} /* namespace crashpad */

int set_annotation(const char *key, const char *value) {
    if (key == nullptr || value == nullptr) {
        return SENTRY_ERROR_NULL_ARGUMENT;
    }
    simple_annotations.SetKeyValue(key, value);
    return 0;
} /* namespace crashpad */

int remove_annotation(const char *key) {
    if (key == nullptr) {
        return SENTRY_ERROR_NULL_ARGUMENT;
    }
    simple_annotations.RemoveKey(key);
    return 0;
}

} /* namespace crashpad */
} /* namespace sentry */
#endif
