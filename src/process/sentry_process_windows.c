#include "sentry_process.h"

#include "sentry_alloc.h"
#include "sentry_logger.h"

#include <stdarg.h>
#include <windows.h>

static size_t
quote_arg(const wchar_t *src, wchar_t *dst)
{
    if (!wcschr(src, L' ') && !wcschr(src, L'"')) {
        if (dst) {
            wcscpy(dst, src);
        }
        return wcslen(src);
    }

    size_t len = 0;

    // opening quote
    if (dst) {
        dst[len] = L'"';
    }
    len++;

    // escape quotes and backslashes
    for (const wchar_t *p = src; *p; ++p) {
        if (*p == L'"') {
            if (dst) {
                dst[len] = L'\\';
                dst[len + 1] = L'"';
            }
            len += 2;
        } else if (*p == L'\\') {
            const wchar_t *q = p;
            size_t slashes = 0;
            while (*q == L'\\') {
                slashes++;
                q++;
            }
            if (*q == L'"') {
                // backslashes followed by a quote -> double the backslashes to
                // escape each backslash
                slashes *= 2;
            }
            for (size_t i = 0; i < slashes; ++i) {
                if (dst) {
                    dst[len] = L'\\';
                }
                len++;
            }
            p = q - 1;
        } else {
            if (dst) {
                dst[len] = *p;
            }
            len++;
        }
    }

    // closing quote
    if (dst) {
        dst[len] = L'"';
        dst[len + 1] = L'\0';
    }
    return ++len;
}

void
sentry__process_spawn(const sentry_path_t *executable, const wchar_t *arg0, ...)
{
    if (!executable || !executable->path
        || wcscmp(executable->path, L"") == 0) {
        return;
    }

    size_t cli_len = quote_arg(executable->path, NULL) + 1; // \0
    if (arg0) {
        cli_len += quote_arg(arg0, NULL) + 1; // space
        va_list args;
        va_start(args, arg0);
        const wchar_t *argn;
        while ((argn = va_arg(args, const wchar_t *)) != NULL) {
            cli_len += quote_arg(argn, NULL) + 1; // space
        }
        va_end(args);
    }

    wchar_t *cli = sentry_malloc(cli_len * sizeof(wchar_t));
    if (!cli) {
        return;
    }

    size_t offset = quote_arg(executable->path, cli);
    if (arg0) {
        cli[offset++] = L' ';
        offset += quote_arg(arg0, cli + offset);
        va_list args;
        va_start(args, arg0);
        const wchar_t *argn;
        while ((argn = va_arg(args, const wchar_t *)) != NULL) {
            cli[offset++] = L' ';
            offset += quote_arg(argn, cli + offset);
        }
        va_end(args);
    }
    cli[offset] = L'\0';

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
