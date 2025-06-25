#include "sentry_process.h"

#include "sentry_logger.h"

bool
sentry__spawn_process(const sentry_path_t *executable, ...)
{
    SENTRY_DEBUGF("spawning \%" SENTRY_PATH_PRI, executable->path);

#ifdef SENTRY_PLATFORM_WINDOWS
    size_t cli_len = wcslen(executable->path);
    va_list args;
    va_start(args, executable);
    const wchar_t *arg;
    while ((arg = va_arg(args, const wchar_t *)) != NULL) {
        cli_len += 1 + wcslen(arg); // space + argument
    }
    va_end(args);

    wchar_t *cli = sentry_malloc((cli_len + 1) * sizeof(wchar_t));
    if (!cli) {
        return false;
    }
    wcscpy(cli, executable->path);
    va_start(args, executable);
    while ((arg = va_arg(args, const wchar_t *)) != NULL) {
        wcscat(cli, L" ");
        wcscat(cli, arg);
    }
    va_end(args);

    STARTUPINFOW si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    si.cb = sizeof(si);

    BOOL rv = CreateProcessW(NULL, // lpApplicationName
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
        return false;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
#else
    // TODO: POSIX
    return false;
#endif
}
