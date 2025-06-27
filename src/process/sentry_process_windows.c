#include "sentry_process.h"

#include "sentry_alloc.h"
#include "sentry_logger.h"

#include <stdarg.h>
#include <string.h>
#include <windows.h>

struct sentry_process_s {
    sentry_path_t *executable;
    wchar_t *command_line;
    wchar_t *environment_block;
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

    sentry_free(process->command_line);
    sentry_free(process->environment_block);
    sentry__path_free(process->executable);
    sentry_free(process);
}

void
sentry__process_set_env(
    sentry_process_t *process, const wchar_t *key0, const wchar_t *value0, ...)
{
    if (!process || !key0 || !value0) {
        return;
    }

    size_t env_len = 1; // block null-terminator
    env_len += wcslen(key0) + 1 + wcslen(value0) + 1; // "KEY=VALUE\0"

    va_list args;
    va_start(args, value0);
    const wchar_t *key, *value;
    while ((key = va_arg(args, const wchar_t *)) != NULL
        && (value = va_arg(args, const wchar_t *)) != NULL) {
        env_len += wcslen(key) + 1 + wcslen(value) + 1; // "KEY=VALUE\0"
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

    sentry_free(process->environment_block);
    process->environment_block = sentry_malloc(env_len * sizeof(wchar_t));
    if (!process->environment_block) {
        if (env_current) {
            FreeEnvironmentStringsW(env_current);
        }
        return;
    }

    wchar_t *dest = process->environment_block;
    dest += swprintf(dest, env_len - (dest - process->environment_block),
        L"%s=%s", key0, value0);
    dest++; // null-terminator

    va_start(args, value0);
    while ((key = va_arg(args, const wchar_t *)) != NULL
        && (value = va_arg(args, const wchar_t *)) != NULL) {
        dest += swprintf(dest, env_len - (dest - process->environment_block),
            L"%s=%s", key, value);
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

static void
build_command_line(sentry_process_t *process, const wchar_t *executable_path)
{
    sentry_free(process->command_line);

    size_t len = wcslen(executable_path) + 1; // +1 for null terminator
    process->command_line = sentry_malloc(len * sizeof(wchar_t));
    if (!process->command_line) {
        return;
    }

    wcscpy(process->command_line, executable_path);
}

static void
append_argument(sentry_process_t *process, const wchar_t *arg)
{
    if (!process->command_line || !arg) {
        return;
    }

    size_t current_len = wcslen(process->command_line);
    size_t arg_len = wcslen(arg);
    size_t new_len
        = current_len + 1 + arg_len + 1; // space + arg + null terminator

    wchar_t *new_command_line = sentry_malloc(new_len * sizeof(wchar_t));
    if (!new_command_line) {
        return;
    }

    wcscpy(new_command_line, process->command_line);
    wcscat(new_command_line, L" ");
    wcscat(new_command_line, arg);

    sentry_free(process->command_line);
    process->command_line = new_command_line;
}

bool
sentry__process_spawn(sentry_process_t *process)
{
    if (!process || !process->executable) {
        return false;
    }

    SENTRY_DEBUGF("spawning \"%S\"",
        process->command_line ? process->command_line
                              : process->executable->path);

    STARTUPINFOW si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    si.cb = sizeof(si);

    BOOL rv = CreateProcessW(NULL, // lpApplicationName
        process->command_line ? process->command_line
                              : process->executable->path, // lpCommandLine
        NULL, // lpProcessAttributes
        NULL, // lpThreadAttributes
        FALSE, // bInheritHandles
        DETACHED_PROCESS | CREATE_UNICODE_ENVIRONMENT, // dwCreationFlags
        process->environment_block, // lpEnvironment
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
}

bool
sentry__process_spawn_with_args(
    sentry_process_t *process, const wchar_t *arg0, ...)
{
    if (!process || !arg0) {
        return false;
    }

    build_command_line(process, process->executable->path);
    if (!process->command_line) {
        return false;
    }

    append_argument(process, arg0);

    va_list args;
    va_start(args, arg0);
    const wchar_t *argn;
    while ((argn = va_arg(args, const wchar_t *)) != NULL) {
        append_argument(process, argn);
    }
    va_end(args);

    return sentry__process_spawn(process);
}
