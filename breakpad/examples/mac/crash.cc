#include <stdio.h>
#include "client/mac/handler/exception_handler.h"

namespace
{

bool callback(const char *dump_dir,
              const char *minidump_id,
              void *context,
              bool succeeded)
{
    if (succeeded)
    {
        printf("%s/%s.dmp\n", dump_dir, minidump_id);
    }
    else
    {
        printf("ERROR creating minidump\n");
    }

    return succeeded;
}

void crash()
{
    volatile int *i = reinterpret_cast<int *>(0x45);
    *i = 5; // crash!
}

void start()
{
    crash(); // should get inlined with optimizations
}

} // namespace

int main(int argc, char **argv)
{
    google_breakpad::ExceptionHandler eh(".", 0, callback, 0, true, 0);
    start(); // should get inlined with optimizations
    return 0;
}
