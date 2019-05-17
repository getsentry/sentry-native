#include <map>
#include <string>
#include <vector>

#include "client/crashpad_client.h"
#include "client/settings.h"

using namespace crashpad;

int main() {
  // Cache directory that will store crashpad information and minidumps
  base::FilePath database("./crashpad.db");
  // Path to the out-of-process handler executable
  base::FilePath handler("./bin/Release/crashpad_handler");
  // URL used to submit minidumps to
  std::string url("https://sentry.io/api/YYY/minidump/?sentry_key=XXX");
  // Optional annotations passed via --annotations to the handler
  std::map<std::string, std::string> annotations;
  // Optional arguments to pass to the handler
  std::vector<std::string> arguments;

  CrashpadClient client;
  bool success = client.StartHandler(
    handler,
    database,
    database,
    url,
    annotations,
    arguments,
    /* restartable */ true,
    /* asynchronous_start */ false
  );

//  if (success) {
//    success = client.WaitForHandlerStart(10000);
//  }

  return success;
}

