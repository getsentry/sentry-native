#include "sentry_boot.h"

#if defined(SENTRY_PLATFORM_WINDOWS)

#    include <dbghelp.h>
#    include <windows.h>

#    include "sentry.h"
#    include "sentry_logger.h"
#    include "sentry_minidump_writer.h"

#    pragma comment(lib, "dbghelp.lib")

/**
 * Windows minidump writer
 * Windows provides MiniDumpWriteDump API which does all the heavy lifting!
 */
int
sentry__write_minidump(
    const sentry_crash_context_t *ctx, const char *output_path)
{
    SENTRY_DEBUGF("writing minidump to %s", output_path);

    // Open output file
    HANDLE file_handle = CreateFileA(output_path, GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (file_handle == INVALID_HANDLE_VALUE) {
        SENTRY_WARNF("failed to create minidump file: %lu", GetLastError());
        return -1;
    }

    // Open crashed process
    HANDLE process_handle
        = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ctx->crashed_pid);

    if (process_handle == NULL) {
        SENTRY_WARNF("failed to open process %lu: %lu", ctx->crashed_pid,
            GetLastError());
        CloseHandle(file_handle);
        DeleteFileA(output_path);
        return -1;
    }

    // Prepare exception information
    MINIDUMP_EXCEPTION_INFORMATION exception_info = { 0 };
    exception_info.ThreadId = ctx->crashed_tid;
    exception_info.ExceptionPointers
        = (PEXCEPTION_POINTERS)&ctx->platform.exception_record;
    exception_info.ClientPointers = FALSE;

    // Determine minidump type based on configuration
    MINIDUMP_TYPE dump_type;
    switch (ctx->minidump_mode) {
    case SENTRY_MINIDUMP_MODE_STACK_ONLY:
        dump_type = MiniDumpNormal;
        break;

    case SENTRY_MINIDUMP_MODE_SMART:
        dump_type
            = MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithDataSegs;
        break;

    case SENTRY_MINIDUMP_MODE_FULL:
        dump_type = MiniDumpWithFullMemory | MiniDumpWithHandleData
            | MiniDumpWithThreadInfo;
        break;

    default:
        dump_type = MiniDumpNormal;
        break;
    }

    // Write minidump using Windows API
    BOOL success = MiniDumpWriteDump(process_handle, ctx->crashed_pid,
        file_handle, dump_type, &exception_info, NULL, NULL);

    DWORD error = GetLastError();

    CloseHandle(process_handle);
    CloseHandle(file_handle);

    if (!success) {
        SENTRY_WARNF("MiniDumpWriteDump failed: %lu", error);
        DeleteFileA(output_path);
        return -1;
    }

    SENTRY_INFO("successfully wrote minidump");
    return 0;
}

#endif // SENTRY_PLATFORM_WINDOWS
