#include "sentry_crash_context.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <werapi.h>
#include <windows.h>

static PCWSTR get_report_id(const WER_RUNTIME_EXCEPTION_INFORMATION *info);

static void
wer_debugf(const char *fmt, ...)
{
    char msg[1024];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);
    if (n < 0) {
        return;
    }
    msg[sizeof(msg) - 1] = '\0';
    OutputDebugStringA(msg);
}

static int
wer_wide_to_utf8(const wchar_t *src, char *dst, int dst_size)
{
    if (!src || !dst || dst_size <= 0) {
        return 0;
    }
    int written
        = WideCharToMultiByte(CP_UTF8, 0, src, -1, dst, dst_size, NULL, NULL);
    if (written <= 0) {
        dst[0] = '\0';
        return 0;
    }
    dst[dst_size - 1] = '\0';
    return written;
}

static BOOL
wer_buffer_contains(const unsigned char *buf, size_t buf_len,
    const unsigned char *needle, size_t needle_len)
{
    if (!buf || !needle || needle_len == 0 || buf_len < needle_len) {
        return FALSE;
    }

    for (size_t i = 0; i <= buf_len - needle_len; i++) {
        if (memcmp(buf + i, needle, needle_len) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL
wer_file_contains_report_id(const wchar_t *path, const wchar_t *report_id)
{
    if (!path || !report_id) {
        return FALSE;
    }

    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    unsigned char buf[128 * 1024];
    DWORD read = 0;
    BOOL ok = ReadFile(file, buf, sizeof(buf), &read, NULL);
    CloseHandle(file);
    if (!ok || read == 0) {
        return FALSE;
    }

    char report_id_utf8[128];
    if (!wer_wide_to_utf8(report_id, report_id_utf8, sizeof(report_id_utf8))) {
        return FALSE;
    }
    size_t utf8_len = strlen(report_id_utf8);

    unsigned char report_id_utf16[256];
    size_t wide_len = wcslen(report_id);
    if (wide_len * 2 > sizeof(report_id_utf16)) {
        wide_len = sizeof(report_id_utf16) / 2;
    }
    for (size_t i = 0; i < wide_len; i++) {
        wchar_t ch = report_id[i];
        report_id_utf16[i * 2] = (unsigned char)(ch & 0xFF);
        report_id_utf16[i * 2 + 1] = (unsigned char)((ch >> 8) & 0xFF);
    }
    size_t utf16_len = wide_len * 2;

    return wer_buffer_contains(buf, (size_t)read,
               (const unsigned char *)report_id_utf8, utf8_len)
        || wer_buffer_contains(buf, (size_t)read, report_id_utf16, utf16_len);
}

static BOOL
wer_temp_has_report_file(const wchar_t *temp_dir, const wchar_t *report_id)
{
    wchar_t pattern[MAX_PATH];
    if (swprintf(pattern, MAX_PATH, L"%s\\*.WERInternalMetadata.xml", temp_dir)
        < 0) {
        return FALSE;
    }

    WIN32_FIND_DATAW data;
    HANDLE find = FindFirstFileW(pattern, &data);
    if (find == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    BOOL found = FALSE;
    do {
        wchar_t path[MAX_PATH];
        if (swprintf(path, MAX_PATH, L"%s\\%s", temp_dir, data.cFileName) < 0) {
            continue;
        }
        if (wer_file_contains_report_id(path, report_id)) {
            found = TRUE;
            break;
        }
    } while (FindNextFileW(find, &data));

    FindClose(find);
    return found;
}

static BOOL
wer_root_has_report_file(const wchar_t *root_dir, const wchar_t *report_id)
{
    wchar_t pattern[MAX_PATH];
    if (swprintf(pattern, MAX_PATH, L"%s\\*", root_dir) < 0) {
        return FALSE;
    }

    WIN32_FIND_DATAW data;
    HANDLE find = FindFirstFileW(pattern, &data);
    if (find == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    BOOL found = FALSE;
    do {
        if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            || wcscmp(data.cFileName, L".") == 0
            || wcscmp(data.cFileName, L"..") == 0) {
            continue;
        }

        wchar_t report_path[MAX_PATH];
        if (swprintf(report_path, MAX_PATH, L"%s\\%s\\Report.wer", root_dir,
                data.cFileName)
            < 0) {
            continue;
        }
        if (wer_file_contains_report_id(report_path, report_id)) {
            found = TRUE;
            break;
        }
    } while (FindNextFileW(find, &data));

    FindClose(find);
    return found;
}

static void
wer_log_report_presence(
    const char *where, const WER_RUNTIME_EXCEPTION_INFORMATION *exception_info)
{
    PCWSTR report_id = get_report_id(exception_info);
    if (!report_id) {
        wer_debugf("### WER: %s report_presence report_id=absent\n", where);
        return;
    }

    wchar_t program_data[MAX_PATH];
    DWORD len = GetEnvironmentVariableW(
        L"ProgramData", program_data, sizeof(program_data) / sizeof(wchar_t));
    if (len == 0 || len >= sizeof(program_data) / sizeof(wchar_t)) {
        wer_debugf("### WER: %s report_presence report_id=present "
                   "program_data=missing\n",
            where);
        return;
    }

    wchar_t temp_dir[MAX_PATH];
    wchar_t queue_dir[MAX_PATH];
    wchar_t archive_dir[MAX_PATH];
    if (swprintf(temp_dir, MAX_PATH, L"%s\\Microsoft\\Windows\\WER\\Temp",
            program_data)
            < 0
        || swprintf(queue_dir, MAX_PATH,
               L"%s\\Microsoft\\Windows\\WER\\ReportQueue", program_data)
            < 0
        || swprintf(archive_dir, MAX_PATH,
               L"%s\\Microsoft\\Windows\\WER\\ReportArchive", program_data)
            < 0) {
        return;
    }

    BOOL temp_found = wer_temp_has_report_file(temp_dir, report_id);
    BOOL queue_found = wer_root_has_report_file(queue_dir, report_id);
    BOOL archive_found = wer_root_has_report_file(archive_dir, report_id);

    char report_id_utf8[128];
    wer_wide_to_utf8(report_id, report_id_utf8, sizeof(report_id_utf8));
    wer_debugf("### WER: %s report_presence report_id=%s temp=%d queue=%d "
               "archive=%d\n",
        where, report_id_utf8[0] ? report_id_utf8 : "(conversion-failed)",
        (int)temp_found, (int)queue_found, (int)archive_found);
}

static BOOL
wer_copy_wstring(PWSTR dst, PDWORD dst_size, const wchar_t *src)
{
    if (!dst_size || !src) {
        return FALSE;
    }

    size_t required = wcslen(src) + 1;
    if (*dst_size < required || !dst) {
        *dst_size = (DWORD)required;
        return FALSE;
    }

    for (size_t i = 0; i < required; i++) {
        dst[i] = src[i];
    }
    return TRUE;
}

static BOOL
is_fatal_wer_exception(const WER_RUNTIME_EXCEPTION_INFORMATION *info)
{
    // bIsFatal is missing in older SDKs; guard access with dwSize.
    typedef struct {
        DWORD dwSize;
        HANDLE hProcess;
        HANDLE hThread;
        EXCEPTION_RECORD exceptionRecord;
        CONTEXT context;
        PCWSTR pwszReportId;
        BOOL bIsFatal;
        DWORD dwReserved;
    } WER_RUNTIME_EXCEPTION_INFORMATION_19041;

    if (!info
        || info->dwSize
            <= offsetof(WER_RUNTIME_EXCEPTION_INFORMATION_19041, bIsFatal)) {
        return FALSE;
    }

    return ((const WER_RUNTIME_EXCEPTION_INFORMATION_19041 *)info)->bIsFatal;
}

static PCWSTR
get_report_id(const WER_RUNTIME_EXCEPTION_INFORMATION *info)
{
    typedef struct {
        DWORD dwSize;
        HANDLE hProcess;
        HANDLE hThread;
        EXCEPTION_RECORD exceptionRecord;
        CONTEXT context;
        PCWSTR pwszReportId;
    } WER_RUNTIME_EXCEPTION_INFORMATION_WITH_REPORT_ID;

    if (!info
        || info->dwSize
            <= offsetof(WER_RUNTIME_EXCEPTION_INFORMATION_WITH_REPORT_ID,
                pwszReportId)) {
        return NULL;
    }

    return ((const WER_RUNTIME_EXCEPTION_INFORMATION_WITH_REPORT_ID *)info)
        ->pwszReportId;
}

static BOOL
read_registration(
    HANDLE process, PVOID context, sentry_wer_registration_t *registration)
{
    if (!process || !context || !registration) {
        return FALSE;
    }

    if (!ReadProcessMemory(
            process, context, registration, sizeof(*registration), NULL)) {
        return FALSE;
    }

    return registration->version == 1 && registration->app_pid != 0;
}

static BOOL
open_native_crash_objects(const sentry_wer_registration_t *registration,
    HANDLE *mapping, HANDLE *event, sentry_crash_context_t **ctx)
{
    wchar_t shm_name[SENTRY_CRASH_IPC_NAME_SIZE];
    wchar_t event_name[SENTRY_CRASH_IPC_NAME_SIZE];

    swprintf(shm_name, SENTRY_CRASH_IPC_NAME_SIZE,
        L"Local\\SentryCrash-%lu-%llx", (unsigned long)registration->app_pid,
        (unsigned long long)registration->app_tid);
    swprintf(event_name, SENTRY_CRASH_IPC_NAME_SIZE,
        L"Local\\SentryCrashEvent-%lu-%llx",
        (unsigned long)registration->app_pid,
        (unsigned long long)registration->app_tid);

    *mapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, shm_name);
    if (!*mapping) {
        return FALSE;
    }

    *ctx = MapViewOfFile(
        *mapping, FILE_MAP_ALL_ACCESS, 0, 0, SENTRY_CRASH_SHM_SIZE);
    if (!*ctx) {
        CloseHandle(*mapping);
        *mapping = NULL;
        return FALSE;
    }

    if ((*ctx)->magic != SENTRY_CRASH_MAGIC) {
        UnmapViewOfFile(*ctx);
        CloseHandle(*mapping);
        *ctx = NULL;
        *mapping = NULL;
        return FALSE;
    }

    *event = OpenEventW(EVENT_MODIFY_STATE, FALSE, event_name);
    if (!*event) {
        UnmapViewOfFile(*ctx);
        CloseHandle(*mapping);
        *ctx = NULL;
        *mapping = NULL;
        return FALSE;
    }

    return TRUE;
}

static BOOL
process_wer_exception(
    PVOID context, const WER_RUNTIME_EXCEPTION_INFORMATION *exception_info)
{
    if (!exception_info || !is_fatal_wer_exception(exception_info)) {
        wer_debugf("### WER: process_wer_exception ignored fatal=%d\n",
            exception_info ? (int)is_fatal_wer_exception(exception_info) : -1);
        return FALSE;
    }

    sentry_wer_registration_t registration = { 0 };
    if (!read_registration(exception_info->hProcess, context, &registration)) {
        wer_debugf("### WER: read_registration failed\n");
        return FALSE;
    }
    wer_debugf("### WER: registration pid=%lu tid=%llu\n",
        (unsigned long)registration.app_pid,
        (unsigned long long)registration.app_tid);

    HANDLE mapping = NULL;
    HANDLE event = NULL;
    sentry_crash_context_t *ctx = NULL;
    if (!open_native_crash_objects(&registration, &mapping, &event, &ctx)) {
        wer_debugf("### WER: open_native_crash_objects failed\n");
        return FALSE;
    }

    BOOL claimed = FALSE;

    // Extract WER report ID regardless of who claims the crash, so the
    // daemon can include it in the sentry event.
    PCWSTR report_id = get_report_id(exception_info);
    if (report_id) {
        WideCharToMultiByte(CP_UTF8, 0, report_id, -1,
            ctx->platform.wer_report_id,
            (int)sizeof(ctx->platform.wer_report_id), NULL, NULL);
        wer_debugf("### WER: report_id available and copied\n");
    } else {
        wer_debugf("### WER: report_id not yet available in callback\n");
    }

    long state_before = InterlockedCompareExchange(&ctx->state, 0, 0);
    wer_debugf("### WER: state before claim=%ld\n", state_before);

    if (InterlockedCompareExchange(&ctx->state, SENTRY_CRASH_STATE_PROCESSING,
            SENTRY_CRASH_STATE_READY)
        == SENTRY_CRASH_STATE_READY) {
        ctx->crashed_pid = GetProcessId(exception_info->hProcess);
        ctx->crashed_tid = GetThreadId(exception_info->hThread);
        ctx->platform.exception_code
            = exception_info->exceptionRecord.ExceptionCode;
        ctx->platform.exception_record = exception_info->exceptionRecord;
        ctx->platform.context = exception_info->context;
        ctx->platform.exception_pointers = NULL;
        ctx->platform.num_threads = 1;
        ctx->platform.threads[0].thread_id = ctx->crashed_tid;
        ctx->platform.threads[0].context = exception_info->context;

        InterlockedExchange(&ctx->state, SENTRY_CRASH_STATE_CRASHED);
        claimed = TRUE;
        wer_debugf("### WER: claimed crash and set state=CRASHED\n");
    } else {
        wer_debugf("### WER: did not claim crash\n");
    }

    // Always attempt to signal the daemon. If the crash handler claimed
    // the crash first, it already SetEvent'd via sentry__crash_ipc_notify,
    // but a redundant SetEvent on an auto-reset event is harmless.
    if (SetEvent(event)) {
        wer_debugf("### WER: SetEvent succeeded\n");
    } else {
        wer_debugf("### WER: SetEvent failed error=%lu\n", GetLastError());
    }

    CloseHandle(event);
    UnmapViewOfFile(ctx);
    CloseHandle(mapping);
    return claimed;
}

BOOL WINAPI
DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    (void)instance;
    (void)reason;
    (void)reserved;
    return TRUE;
}

HRESULT WINAPI
OutOfProcessExceptionEventCallback(PVOID context,
    const PWER_RUNTIME_EXCEPTION_INFORMATION exception_info,
    BOOL *ownership_claimed, PWSTR event_name, PDWORD event_name_size,
    PDWORD signature_count)
{
    (void)event_name;
    (void)event_name_size;
    (void)signature_count;

    PCWSTR report_id = get_report_id(exception_info);
    char report_id_utf8[128] = { 0 };
    if (report_id) {
        wer_wide_to_utf8(report_id, report_id_utf8, sizeof(report_id_utf8));
    }
    wer_debugf(
        "### WER: OutOfProcessExceptionEventCallback enter report_id=%s\n",
        report_id ? report_id_utf8 : "absent");
    wer_log_report_presence(
        "OutOfProcessExceptionEventCallback.enter", exception_info);

    if (ownership_claimed) {
        *ownership_claimed = FALSE;
    }
    if (process_wer_exception(context, exception_info)) {
        if (ownership_claimed) {
            *ownership_claimed = TRUE;
        }
        if (signature_count) {
            *signature_count = 2;
        }
        if (!wer_copy_wstring(
                event_name, event_name_size, L"SentryNativeCrash")) {
            wer_debugf("### WER: OutOfProcessExceptionEventCallback could not "
                       "set event_name\n");
        }
    }

    wer_debugf("### WER: OutOfProcessExceptionEventCallback exit claimed=%d\n",
        ownership_claimed ? (int)(*ownership_claimed) : 0);
    wer_log_report_presence(
        "OutOfProcessExceptionEventCallback.exit", exception_info);

    return S_OK;
}

HRESULT WINAPI
OutOfProcessExceptionEventSignatureCallback(PVOID context,
    const PWER_RUNTIME_EXCEPTION_INFORMATION exception_info, DWORD index,
    PWSTR name, PDWORD name_size, PWSTR value, PDWORD value_size)
{
    (void)context;
    PCWSTR report_id = get_report_id(exception_info);
    char report_id_utf8[128] = { 0 };
    if (report_id) {
        wer_wide_to_utf8(report_id, report_id_utf8, sizeof(report_id_utf8));
    }
    wer_debugf("### WER: OutOfProcessExceptionEventSignatureCallback "
               "index=%lu report_id=%s\n",
        (unsigned long)index, report_id ? report_id_utf8 : "absent");
    wer_log_report_presence(
        "OutOfProcessExceptionEventSignatureCallback", exception_info);

    const wchar_t *name_src = NULL;
    wchar_t value_buf[64];
    const wchar_t *value_src = NULL;

    if (index == 0) {
        name_src = L"ReportId";
        value_src = report_id ? report_id : L"(absent)";
    } else if (index == 1) {
        name_src = L"ExceptionCode";
        DWORD code = exception_info
            ? exception_info->exceptionRecord.ExceptionCode
            : 0;
        swprintf(value_buf, sizeof(value_buf) / sizeof(wchar_t), L"0x%08lx",
            (unsigned long)code);
        value_src = value_buf;
    } else {
        return E_INVALIDARG;
    }

    if (!wer_copy_wstring(name, name_size, name_src)
        || !wer_copy_wstring(value, value_size, value_src)) {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    return S_OK;
}

HRESULT WINAPI
OutOfProcessExceptionEventDebuggerLaunchCallback(PVOID context,
    const PWER_RUNTIME_EXCEPTION_INFORMATION exception_info,
    PBOOL is_custom_debugger, PWSTR debugger_launch,
    PDWORD debugger_launch_size, PBOOL is_debugger_autolaunch)
{
    (void)context;
    (void)exception_info;
    PCWSTR report_id = get_report_id(exception_info);
    char report_id_utf8[128] = { 0 };
    if (report_id) {
        wer_wide_to_utf8(report_id, report_id_utf8, sizeof(report_id_utf8));
    }
    wer_debugf("### WER: OutOfProcessExceptionEventDebuggerLaunchCallback "
               "report_id=%s\n",
        report_id ? report_id_utf8 : "absent");
    wer_log_report_presence(
        "OutOfProcessExceptionEventDebuggerLaunchCallback", exception_info);

    if (is_custom_debugger) {
        *is_custom_debugger = FALSE;
    }
    if (is_debugger_autolaunch) {
        *is_debugger_autolaunch = FALSE;
    }
    if (debugger_launch && debugger_launch_size && *debugger_launch_size > 0) {
        debugger_launch[0] = L'\0';
    }
    if (debugger_launch_size) {
        *debugger_launch_size = 0;
    }

    return S_OK;
}
