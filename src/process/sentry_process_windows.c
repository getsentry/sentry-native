#include "sentry_process.h"

#include "sentry_alloc.h"
#include "sentry_logger.h"

#include <stdarg.h>
#include <windows.h>

void
sentry__process_spawn(const sentry_path_t *executable, const wchar_t *arg0, ...)
{
    if (!executable || !executable->path
        || wcscmp(executable->path, L"") == 0) {
        return;
    }

    size_t cli_len = wcslen(executable->path) + 1; // \0
    if (arg0) {
        cli_len += wcslen(arg0) + 1; // space
        va_list args;
        va_start(args, arg0);
        const wchar_t *argn;
        while ((argn = va_arg(args, const wchar_t *)) != NULL) {
            cli_len += wcslen(argn) + 1; // space
        }
        va_end(args);
    }

    wchar_t *cli = sentry_malloc(cli_len * sizeof(wchar_t));
    if (!cli) {
        return;
    }
    wcscpy(cli, executable->path);
    if (arg0) {
        wcscat(cli, L" ");
        wcscat(cli, arg0);
        va_list args;
        va_start(args, arg0);
        const wchar_t *argn;
        while ((argn = va_arg(args, const wchar_t *)) != NULL) {
            wcscat(cli, L" ");
            wcscat(cli, argn);
        }
        va_end(args);
    }

    SENTRY_DEBUGF("spawning %S", cli);

    STARTUPINFOW si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    si.cb = sizeof(si);

    BOOL rv = CreateProcessW(executable->path, // lpApplicationName
        cli, // lpCommandLine
        NULL, // lpProcessAttributes
        NULL, // lpThreadAttributes
        FALSE, // bInheritHandles
        DETACHED_PROCESS, // dwCreationFlags
        NULL, // lpEnvironment
        NULL, // lpCurrentDirectory
        &si, // lpStartupInfo
        &pi // lpProcessInformation
    );

    sentry_free(cli);

    if (!rv) {
        SENTRY_ERRORF("CreateProcess failed: %lu", GetLastError());
        return;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}
