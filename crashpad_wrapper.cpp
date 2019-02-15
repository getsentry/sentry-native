#include <stdio.h>
#include <map>
#include <string>
#include <vector>

#include "client/crashpad_client.h"
#include "client/settings.h"
#include "client/crash_report_database.h"

using namespace crashpad;

int main()
{
    // Cache directory that will store crashpad information and minidumps
    base::FilePath database(".");
    // Path to the out-of-process handler executable
    base::FilePath handler("../crashpad-Darwin/bin/crashpad_handler");
    // URL used to submit minidumps to
    std::string url("https://sentry.io/api/1188141/minidump/?sentry_key=5fd7a6cda8444965bade9ccfd3df9882");
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
        crashpad::CrashReportDatabase::Initialize(database);

    if (db != nullptr && db->GetSettings() != nullptr)
    {
        db->GetSettings()->SetUploadsEnabled(true);
    }

    std::function<void(int)> func = nullptr;
    func(1);

    return success;
}