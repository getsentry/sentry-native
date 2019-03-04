#include <stdio.h>
#include <map>
#include <string>
#include <vector>
#include <atomic>

#include "sentry.h"
#include "client/crashpad_client.h"
#include "client/settings.h"
#include "client/crash_report_database.h"
#include "client/crashpad_info.h"

using namespace crashpad;

namespace sentry
{
namespace crashpad
{
SimpleStringDictionary simple_annotations;

int init(const sentry_options_t *options, const char *minidump_url)
{
    if (minidump_url == nullptr)
    {
        return SENTRY_ERROR_NO_MINIDUMP_URL;
    }

    // Cache directory that will store crashpad information and minidumps
    base::FilePath database(options->database_path);
    // Path to the out-of-process handler executable
    base::FilePath handler(options->handler_path);
    // URL used to submit minidumps to
    std::string url(minidump_url);
    // Optional annotations passed via --annotations to the handler
    std::map<std::string, std::string> annotations;
    // Optional arguments to pass to the handler
    std::vector<std::string> arguments;
    arguments.push_back("--no-rate-limit");

    CrashpadClient client;
    bool success = client.StartHandler(
        handler,
        database,
        database,
        url,
        annotations,
        arguments,
        /* restartable */ true,
        /* asynchronous_start */ false);

    if (options->debug)
    {
        if (success)
        {
            printf("Started client handler.");
        }
        {
            printf("Failed to start client handler.");
        }
    }

    if (!success)
    {
        return SENTRY_ERROR_HANDLER_STARTUP_FAIL;
    }

    std::unique_ptr<CrashReportDatabase> db =
        CrashReportDatabase::Initialize(database);

    if (db != nullptr && db->GetSettings() != nullptr)
    {
        db->GetSettings()->SetUploadsEnabled(true);
    }

    // Ensure that the simple annotations dictionary is set in the client.
    CrashpadInfo *crashpad_info = CrashpadInfo::GetCrashpadInfo();
    crashpad_info->set_simple_annotations(&simple_annotations);

    return SENTRY_SUCCESS;
}

int set_annotation(const char *key, const char *value)
{
    if (key == nullptr || value == nullptr)
    {
        return SENTRY_ERROR_NULL_ARGUMENT;
    }
    simple_annotations.SetKeyValue(key, value);
    return SENTRY_SUCCESS;
} // namespace crashpad

int remove_annotation(const char *key)
{
    if (key == nullptr)
    {
        return SENTRY_ERROR_NULL_ARGUMENT;
    }
    simple_annotations.RemoveKey(key);
    return SENTRY_SUCCESS;
}

} // namespace crashpad
} // namespace sentry