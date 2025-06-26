#include "sentry_process.h"

#include "sentry_alloc.h"
#include "sentry_logger.h"

struct sentry_process_s {
    sentry_path_t *executable;
    sentry_pathchar_t *cli;
    sentry_pathchar_t *env;
};

sentry_process_t *
sentry__process_new(const sentry_path_t *executable)
{
    sentry_process_t *process = SENTRY_MAKE(sentry_process_t);
    if (!process) {
        return NULL;
    }
    memset(process, 0, sizeof(sentry_process_t));

    process->executable = sentry__path_clone(executable);
    if (!process->executable) {
        sentry_free(process);
        return NULL;
    }

    return process;
}

void
sentry__process_free(sentry_process_t *process)
{
    if (!process) {
        return;
    }

    sentry_free(process->cli);
    sentry_free(process->env);
    sentry__path_free(process->executable);
    sentry_free(process);
}

void
sentry__process_set_env(sentry_process_t *process, const sentry_pathchar_t *key,
    const sentry_pathchar_t *value, ...)
{
    if (!process || !key || !value) {
        return;
    }

    size_t env_len = 1; // block null-terminator
    env_len += wcslen(key) + 1 + wcslen(value) + 1; // "KEY=VALUE\0"
    va_list args;
    va_start(args, value);
    const sentry_pathchar_t *k, *v;
    while ((k = va_arg(args, const sentry_pathchar_t *)) != NULL
        && (v = va_arg(args, const sentry_pathchar_t *)) != NULL) {
        env_len += wcslen(k) + 1 + wcslen(v) + 1; // "KEY=VALUE\0"
    }
    va_end(args);

    wchar_t *env_current = GetEnvironmentStringsW();
    if (env_current) {
        wchar_t *ptr = env_current;
        while (*ptr) {
            size_t len = wcslen(ptr) + 1; // string + null-terminator
            env_len += len;
            ptr += len;
        }
    }

    sentry_free(process->env);
    process->env = sentry_malloc(env_len * sizeof(wchar_t));
    if (!process->env) {
        if (env_current) {
            FreeEnvironmentStringsW(env_current);
        }
        return;
    }

    sentry_pathchar_t *dest = process->env;
    dest += swprintf(
        dest, env_len - (dest - process->env), L"%s=%s", key, value);
    dest++; // null-terminator

    va_start(args, value);
    while ((k = va_arg(args, const sentry_pathchar_t *)) != NULL
        && (v = va_arg(args, const sentry_pathchar_t *)) != NULL) {
        dest += swprintf(dest, env_len - (dest - process->env), L"%s=%s", k, v);
        dest++; // null-terminator
    }
    va_end(args);

    if (env_current) {
        wchar_t *ptr = env_current;
        while (*ptr) {
            size_t len = wcslen(ptr) + 1;
            wcscpy(dest, ptr);
            dest += len;
            ptr += len;
        }
        FreeEnvironmentStringsW(env_current);
    }

    *dest = L'\0';
}

bool
sentry__process_spawn(sentry_process_t *process)
{
    if (!process || !process->executable) {
        return false;
    }

    SENTRY_DEBUGF("spawning \"%" SENTRY_PATH_PRI "\"",
        process->cli ? process->cli : process->executable->path);

#ifdef SENTRY_PLATFORM_WINDOWS
    STARTUPINFOW si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    si.cb = sizeof(si);

    BOOL rv = CreateProcessW(NULL, // lpApplicationName
        process->cli ? process->cli
                     : process->executable->path, // lpCommandLine
        NULL, // lpProcessAttributes
        NULL, // lpThreadAttributes
        FALSE, // bInheritHandles
        DETACHED_PROCESS | CREATE_UNICODE_ENVIRONMENT, // dwCreationFlags
        process->env, // lpEnvironment
        NULL, // lpCurrentDirectory
        &si, // lpStartupInfo
        &pi // lpProcessInformation
    );

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

bool
sentry__process_spawn_with_args(
    sentry_process_t *process, const sentry_pathchar_t *arg, ...)
{
    if (!process || !arg) {
        return false;
    }

    size_t cli_len = wcslen(process->executable->path) + 1; // space
    cli_len += wcslen(arg) + 1; // space / null-terminator

    va_list args;
    va_start(args, arg);
    const sentry_pathchar_t *a;
    while ((a = va_arg(args, const sentry_pathchar_t *)) != NULL) {
        cli_len += wcslen(a) + 1; // space / null-terminator
    }
    va_end(args);

    sentry_free(process->cli);
    process->cli = sentry_malloc(cli_len * sizeof(wchar_t));
    if (!process->cli) {
        return;
    }
    wcscpy(process->cli, process->executable->path);
    wcscat(process->cli, L" ");
    wcscat(process->cli, arg);

    va_start(args, arg);
    while ((a = va_arg(args, const sentry_pathchar_t *)) != NULL) {
        wcscat(process->cli, L" ");
        wcscat(process->cli, a);
    }
    va_end(args);

    return sentry__process_spawn(process);
}
