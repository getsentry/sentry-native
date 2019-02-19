#include <stdio.h>
#include <map>
#include <string>
#include <vector>

#include "client/crashpad_client.h"
#include "client/settings.h"
#include "client/crash_report_database.h"
#include "client/crashpad_info.h"

using namespace crashpad;

namespace sentry
{
namespace crashpad
{
int init()
{
    // Cache directory that will store crashpad information and minidumps
    base::FilePath database(".");
    // Path to the out-of-process handler executable
    base::FilePath handler("../crashpad-Darwin/bin/crashpad_handler");
    // URL used to submit minidumps to
    std::string url("https://sentry.garcia.in/api/3/minidump/?sentry_key=93b6c4c0c1a14bec977f0f1adf8525e6");
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

    if (success)
    {
        printf("Started client handler.");
    }
    {
        printf("Failed to start client handler.");
    }

    std::unique_ptr<CrashReportDatabase> db =
        CrashReportDatabase::Initialize(database);

    if (db != nullptr && db->GetSettings() != nullptr)
    {
        db->GetSettings()->SetUploadsEnabled(true);
    }

    return success;
}

int set_tag(const char *key, const char *value)
{
    // TODO: Hold the ref to the dictionary.
    SimpleStringDictionary *annotations = new SimpleStringDictionary();
    annotations->SetKeyValue(key, value);

    CrashpadInfo *crashpad_info = CrashpadInfo::GetCrashpadInfo();
    crashpad_info->set_simple_annotations(annotations);
    return 0;
}

int set_extra(const char *key, const char *value)
{
    // TODO: Hold the ref to the dictionary.
    SimpleStringDictionary *annotations = new SimpleStringDictionary();
    annotations->SetKeyValue(key, value);

    CrashpadInfo *crashpad_info = CrashpadInfo::GetCrashpadInfo();
    crashpad_info->set_simple_annotations(annotations);
    return 0;
}

} // namespace crashpad
} // namespace sentry