#include "sentry_process.h"

#include "sentry_logger.h"

bool
sentry__process_spawn(
    const sentry_pathchar_t **argv, const sentry_pathchar_t **envp)
{
    if (!argv || !argv[0]) {
        return false;
    }

    SENTRY_DEBUGF("spawning \"%" SENTRY_PATH_PRI "\"", argv[0]);

#ifdef SENTRY_PLATFORM_WINDOWS
    size_t cli_len = 0;
    for (const wchar_t **arg = argv; *arg; arg++) {
        cli_len += wcslen(*arg) + 1; // space + null-terminator
    }

    wchar_t *cli = sentry_malloc(cli_len * sizeof(wchar_t));
    if (!cli) {
        return false;
    }
    wcscpy(cli, argv[0]);
    for (const wchar_t **arg = argv + 1; *arg; arg++) {
        wcscat(cli, L" ");
        wcscat(cli, *arg);
    }

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
