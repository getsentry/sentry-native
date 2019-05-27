#include <map>
#include <string>
#include <vector>

#include "client/crashpad_client.h"
#include "client/settings.h"

using namespace crashpad;

// If running in docker, pass "--security-opt seccomp:unconfined"
void crash()
{
    volatile int *i = reinterpret_cast<int *>(0x0);
    *i = 5; // crash!
}

int main() {
  // Cache directory that will store crashpad information and minidumps
  base::FilePath database("./crashpad.db");
  // Path to the out-of-process handler executable
  base::FilePath handler("./bin/Release/crashpad_handler");
  // URL used to submit minidumps to
  std::string url("https://sentry.io/api/YYY/minidump/?sentry_key=XXX");
  // Optional annotations passed via --annotations to the handler
  std::map<std::string, std::string> annotations;
  // File attachments
  std::map<std::string, base::FilePath> fileAttachments;
  // Optional arguments to pass to the handler
  std::vector<std::string> arguments;

  CrashpadClient client;
  bool success = client.StartHandlerWithAttachments(
    handler,
    database,
    database,
    url,
    annotations,
    fileAttachments,
    arguments,
    /* restartable */ true,
    /* asynchronous_start */ false
  );

  if (success) {
    printf("Registered the handler, preparing to crash...\n");
  } else {
    printf("Could not register the handler, exiting\n\n");
    return 1;
  }

  crash();
  return 0;
}
