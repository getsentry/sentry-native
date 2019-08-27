#include <stdio.h>
#include "client/linux/handler/exception_handler.h"

namespace
{

bool callback(const google_breakpad::MinidumpDescriptor &descriptor,
              void *context,
              bool succeeded)
{
    if (succeeded)
    {
        printf("%s\n", descriptor.path());
    }
    else
    {
        printf(
            "ERROR creating minidump. If running in docker, pass "
            "--security-opt "
            "seccomp:unconfined");
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

int main(int argc, char *argv[])
{
    google_breakpad::MinidumpDescriptor descriptor(".");
    google_breakpad::ExceptionHandler eh(descriptor, 0, callback, 0, true, -1);
    start(); // should get inlined with optimizations
    return 0;
}
