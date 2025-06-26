#include "sentry_process.h"

#include "sentry_logger.h"

#ifdef SENTRY_PLATFORM_WINDOWS
static wchar_t *
merge_env(const sentry_pathchar_t **envp)
{
    if (!envp) {
        return NULL;
    }

    // Calculate size for explicitly specified values
    size_t env_len = 1; // block null-terminator
    for (const wchar_t **env = envp; *env; env++) {
        env_len += wcslen(*env) + 1; // string + null-terminator
    }

    // Get current environment and add its size
    wchar_t *current_env = GetEnvironmentStringsW();
    if (current_env) {
        wchar_t *ptr = current_env;
        while (*ptr) {
            env_len += wcslen(ptr) + 1; // string + null-terminator
            ptr += wcslen(ptr) + 1;
        }
    }

    // Allocate environment block
    wchar_t *env_block = sentry_malloc(env_len * sizeof(wchar_t));
    if (!env_block) {
        if (current_env) {
            FreeEnvironmentStringsW(current_env);
        }
        return NULL;
    }

    // Build environment block: first add explicitly specified values
    wchar_t *dest = env_block;
    for (const wchar_t **env = envp; *env; env++) {
        size_t len = wcslen(*env) + 1;
        wcscpy(dest, *env);
        dest += len;
    }

    // Then add all current environment variables
    if (current_env) {
        wchar_t *ptr = current_env;
        while (*ptr) {
            size_t len = wcslen(ptr) + 1;
            wcscpy(dest, ptr);
            dest += len;
            ptr += len;
        }
        FreeEnvironmentStringsW(current_env);
    }

    // Final null-terminator for the block
    *dest = L'\0';

    return env_block;
}
#endif

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
        cli_len += wcslen(*arg) + 1; // space / null-terminator
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

    wchar_t *env_block = merge_env(envp);

    STARTUPINFOW si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    si.cb = sizeof(si);

    BOOL rv = CreateProcessW(NULL, // lpApplicationName
        cli, // lpCommandLine
        NULL, // lpProcessAttributes
        NULL, // lpThreadAttributes
        FALSE, // bInheritHandles
        DETACHED_PROCESS | CREATE_UNICODE_ENVIRONMENT, // dwCreationFlags
        env_block, // lpEnvironment
        NULL, // lpCurrentDirectory
        &si, // lpStartupInfo
        &pi // lpProcessInformation
    );
    sentry_free(cli);
    sentry_free(env_block);

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
