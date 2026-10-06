#include "sentry_os.h"
#include "sentry_path.h"
#include "sentry_slice.h"
#include "sentry_string.h"
#include "sentry_value.h"

#include <stdio.h>
#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_WINDOWS)
#    include "sentry_core.h"
#    include "sentry_logger.h"
#    include "sentry_utils.h"
#endif
#ifdef SENTRY_PLATFORM_LINUX
#    include <unistd.h>
#endif

#ifdef SENTRY_PLATFORM_WINDOWS

#    include <signal.h>
#    include <string.h>

static sentry__win32_abort_handler_t g_sigabrt_handler = NULL;
static void (*g_previous_sigabrt_handler)(int) = NULL;
static bool g_sigabrt_installed = false;

static void
handle_sigabrt(int signum)
{
    (void)signum;

    // Capture the current CPU context
    CONTEXT context;
    RtlCaptureContext(&context);

    // Create a synthetic exception record for abort
    EXCEPTION_RECORD record;
    memset(&record, 0, sizeof(record));
    record.ExceptionCode = STATUS_FATAL_APP_EXIT;
    record.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
#    if defined(_M_AMD64)
    record.ExceptionAddress = (PVOID)context.Rip;
#    elif defined(_M_IX86)
    record.ExceptionAddress = (PVOID)context.Eip;
#    elif defined(_M_ARM64)
    record.ExceptionAddress = (PVOID)context.Pc;
#    endif

    EXCEPTION_POINTERS exception_pointers;
    exception_pointers.ContextRecord = &context;
    exception_pointers.ExceptionRecord = &record;

    if (g_sigabrt_handler) {
        g_sigabrt_handler(&exception_pointers);
    }

    // If we get here, call the previous handler or terminate
    if (g_previous_sigabrt_handler && g_previous_sigabrt_handler != SIG_DFL
        && g_previous_sigabrt_handler != SIG_IGN) {
        g_previous_sigabrt_handler(signum);
    }

    // Terminate the process - abort() must not return
    TerminateProcess(GetCurrentProcess(), 3);
}

void
sentry__win32_install_sigabrt_handler(sentry__win32_abort_handler_t handler)
{
    g_sigabrt_handler = handler;
    void (*previous)(int) = signal(SIGABRT, handle_sigabrt);
    if (previous != SIG_ERR) {
        if (previous != handle_sigabrt) {
            g_previous_sigabrt_handler = previous;
        }
        g_sigabrt_installed = true;
    }
}

void
sentry__win32_restore_sigabrt_handler(void)
{
    if (g_sigabrt_installed) {
        // Restore previous SIGABRT handler (unconditionally, since SIG_DFL is
        // typically NULL on MSVC and a conditional check would skip
        // restoration)
        signal(SIGABRT, g_previous_sigabrt_handler);
        g_sigabrt_installed = false;
    }
    g_previous_sigabrt_handler = NULL;
    g_sigabrt_handler = NULL;
}

#    if !defined(SENTRY_PLATFORM_XBOX)
#        include <stdlib.h>
#        include <windows.h>
#        define CURRENT_VERSION                                                \
            "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"

static void *
try_file_version(const LPCWSTR filename)
{
    const DWORD size = GetFileVersionInfoSizeW(filename, NULL);
    if (!size) {
        return NULL;
    }

    void *ffibuf = sentry_malloc(size);
    if (ffibuf && !GetFileVersionInfoW(filename, 0, size, ffibuf)) {
        sentry_free(ffibuf);
        return NULL;
    }
    return ffibuf;
}

int
sentry__get_kernel_version(windows_version_t *win_ver)
{
    void *ffibuf = try_file_version(L"ntoskrnl.exe");
    if (!ffibuf) {
        ffibuf = try_file_version(L"kernel32.dll");
    }
    if (!ffibuf) {
        return 0;
    }

    VS_FIXEDFILEINFO *ffi;
    UINT ffi_size;
    if (!VerQueryValueW(ffibuf, L"\\", (LPVOID *)&ffi, &ffi_size)) {
        sentry_free(ffibuf);
        return 0;
    }
    ffi->dwFileFlags &= ffi->dwFileFlagsMask;

    win_ver->major = ffi->dwFileVersionMS >> 16;
    win_ver->minor = ffi->dwFileVersionMS & 0xffff;
    win_ver->build = ffi->dwFileVersionLS >> 16;
    win_ver->ubr = ffi->dwFileVersionLS & 0xffff;

    sentry_free(ffibuf);

    return 1;
}

int
sentry__get_windows_version(windows_version_t *win_ver)
{
    // The `CurrentMajorVersionNumber`, `CurrentMinorVersionNumber` and `UBR`
    // are DWORD, while `CurrentBuild` is a SZ (text).
    uint32_t reg_version = 0;
    DWORD buf_size = sizeof(uint32_t);
    if (RegGetValueA(HKEY_LOCAL_MACHINE, CURRENT_VERSION,
            "CurrentMajorVersionNumber", RRF_RT_REG_DWORD, NULL, &reg_version,
            &buf_size)
        != ERROR_SUCCESS) {
        return 0;
    }
    win_ver->major = reg_version;

    buf_size = sizeof(uint32_t);
    if (RegGetValueA(HKEY_LOCAL_MACHINE, CURRENT_VERSION,
            "CurrentMinorVersionNumber", RRF_RT_REG_DWORD, NULL, &reg_version,
            &buf_size)
        != ERROR_SUCCESS) {
        return 0;
    }
    win_ver->minor = reg_version;

    char buf[32];
    buf_size = sizeof(buf);
    if (RegGetValueA(HKEY_LOCAL_MACHINE, CURRENT_VERSION, "CurrentBuild",
            RRF_RT_REG_SZ, NULL, buf, &buf_size)
        != ERROR_SUCCESS) {
        return 0;
    }
    win_ver->build = strtoul(buf, NULL, 10);

    // UBR (Update Build Revision) is optional (not present on Wine)
    reg_version = 0;
    buf_size = sizeof(uint32_t);
    RegGetValueA(HKEY_LOCAL_MACHINE, CURRENT_VERSION, "UBR", RRF_RT_REG_DWORD,
        NULL, &reg_version, &buf_size);
    win_ver->ubr = reg_version;

    return 1;
}

static bool
string_ends_with(const char *value, size_t value_len, const char *suffix)
{
    const size_t suffix_len = strlen(suffix);
    return value_len >= suffix_len
        && memcmp(value + value_len - suffix_len, suffix, suffix_len) == 0;
}

static sentry_path_t *
make_wine_path(const char *path, const char *suffix)
{
    sentry_stringbuilder_t sb;
    sentry__stringbuilder_init(&sb);
    if (sentry__stringbuilder_append(&sb, "Z:")
        || sentry__stringbuilder_append(&sb, path)
        || sentry__stringbuilder_append(&sb, suffix)) {
        sentry__stringbuilder_cleanup(&sb);
        return NULL;
    }
    return sentry__path_from_str_owned(sentry__stringbuilder_into_string(&sb));
}

static char *
read_wine_file(const sentry_path_t *path)
{
    return path ? sentry__path_read_to_buffer(path, NULL) : NULL;
}

static char *
get_proton_version(bool *is_proton)
{
    *is_proton = false;
    char *compat_path
        = sentry__string_from_wstr(_wgetenv(L"STEAM_COMPAT_DATA_PATH"));
    if (sentry__string_empty(compat_path)) {
        sentry_free(compat_path);
        return NULL;
    }

    sentry_path_t *config_path = make_wine_path(compat_path, "/config_info");
    sentry_free(compat_path);
    char *config = config_path ? read_wine_file(config_path) : NULL;
    sentry__path_free(config_path);
    if (!config) {
        return NULL;
    }

    char *fonts_path = strchr(config, '\n');
    if (!fonts_path) {
        sentry_free(config);
        return NULL;
    }
    fonts_path++;
    while (*fonts_path == ' ' || *fonts_path == '\t') {
        fonts_path++;
    }
    size_t fonts_path_len = strcspn(fonts_path, "\r\n");
    while (fonts_path_len > 0
        && (fonts_path[fonts_path_len - 1] == ' '
            || fonts_path[fonts_path_len - 1] == '\t')) {
        fonts_path_len--;
    }

    const char *fonts_suffix = NULL;
    if (string_ends_with(fonts_path, fonts_path_len, "/files/share/fonts/")) {
        fonts_suffix = "/files/share/fonts/";
    } else if (string_ends_with(
                   fonts_path, fonts_path_len, "/dist/share/fonts/")) {
        fonts_suffix = "/dist/share/fonts/";
    }
    if (!fonts_suffix) {
        sentry_free(config);
        return NULL;
    }

    const size_t root_len = fonts_path_len - strlen(fonts_suffix);
    char *proton_root = sentry__string_clone_n(fonts_path, root_len);
    sentry_free(config);
    if (!proton_root) {
        return NULL;
    }

    sentry_path_t *proton_path = make_wine_path(proton_root, "/proton");
    *is_proton = proton_path && sentry__path_is_file(proton_path);
    sentry__path_free(proton_path);

    sentry_path_t *version_path = make_wine_path(proton_root, "/version");
    sentry_free(proton_root);
    char *version_file = version_path ? read_wine_file(version_path) : NULL;
    sentry__path_free(version_path);
    if (!version_file) {
        return NULL;
    }

    // Format: "<timestamp> <git-tag>", e.g. "1769167055 proton-10.0-4".
    char *version = strpbrk(version_file, " \t");
    if (!version) {
        sentry_free(version_file);
        return NULL;
    }
    while (*version == ' ' || *version == '\t') {
        version++;
    }
    size_t version_len = strcspn(version, "\r\n");
    while (version_len > 0
        && (version[version_len - 1] == ' '
            || version[version_len - 1] == '\t')) {
        version_len--;
    }

    char *result
        = version_len ? sentry__string_clone_n(version, version_len) : NULL;
    sentry_free(version_file);
    return result;
}

static bool
string_starts_with(const char *value, const char *prefix)
{
    const size_t value_len = strlen(value);
    const size_t prefix_len = strlen(prefix);
    return value_len >= prefix_len && memcmp(value, prefix, prefix_len) == 0;
}

typedef const char *(CDECL *sentry__wine_get_version_t)(void);

static sentry_value_t
make_wine_context(sentry__wine_get_version_t wine_get_version,
    const char *proton_version, bool is_proton)
{
    if (!wine_get_version) {
        return sentry_value_new_null();
    }

    const char *runtime_name = "Wine";
    const char *runtime_version = wine_get_version();
    if (!sentry__string_empty(proton_version)) {
        runtime_version = proton_version;
        if (is_proton) {
            if (string_starts_with(proton_version, "proton-")) {
                runtime_name = "Proton";
                runtime_version += strlen("proton-");
            } else if (string_starts_with(proton_version, "experimental-")) {
                runtime_name = "Proton Experimental";
                runtime_version += strlen("experimental-");
            } else if (string_starts_with(proton_version, "GE-Proton")) {
                runtime_name = "GE-Proton";
                runtime_version += strlen("GE-Proton");
            } else if (string_starts_with(proton_version, "hotfix-")) {
                runtime_name = "Proton Hotfix";
                runtime_version += strlen("hotfix-");
            } else {
                runtime_name = "Proton Custom";
            }
        }
    }
    if (sentry__string_empty(runtime_version)) {
        return sentry_value_new_null();
    }

    sentry_value_t context = sentry_value_new_object();
    if (sentry_value_is_null(context)) {
        return context;
    }
    sentry_value_set_by_key(
        context, "type", sentry_value_new_string("runtime"));
    sentry_value_set_by_key(
        context, "name", sentry_value_new_string(runtime_name));
    sentry_value_set_by_key(
        context, "version", sentry_value_new_string(runtime_version));
    sentry_value_freeze(context);
    return context;
}

sentry_value_t
sentry__get_wine_context(void)
{
    const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) {
        return sentry_value_new_null();
    }

    const sentry__wine_get_version_t wine_get_version
        = (sentry__wine_get_version_t)GetProcAddress(ntdll, "wine_get_version");
    if (!wine_get_version) {
        return sentry_value_new_null();
    }
    bool is_proton = false;
    char *proton_version = get_proton_version(&is_proton);
    sentry_value_t context
        = make_wine_context(wine_get_version, proton_version, is_proton);
    sentry_free(proton_version);
    return context;
}

#    endif // !defined(SENTRY_PLATFORM_XBOX)

sentry_value_t
sentry__get_os_context(void)
{
    const sentry_value_t os = sentry_value_new_object();
    if (sentry_value_is_null(os)) {
        return os;
    }

#    if defined(SENTRY_PLATFORM_XBOX)
#        pragma warning(push)
#        pragma warning(disable : 4996)
    sentry_value_set_by_key(os, "name", sentry_value_new_string("Xbox"));
    OSVERSIONINFO os_ver = { 0 };
    char buf[128];
    buf[0] = 0;
    os_ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&os_ver);
    snprintf(buf, sizeof(buf), "%u.%u.%u", os_ver.dwMajorVersion,
        os_ver.dwMinorVersion, os_ver.dwBuildNumber);
    sentry_value_set_by_key(os, "version", sentry_value_new_string(buf));

    sentry_value_freeze(os);
    return os;
#        pragma warning(pop)
#    else

    sentry_value_set_by_key(os, "name", sentry_value_new_string("Windows"));

    bool at_least_one_key_successful = false;
    char buf[32];
    windows_version_t win_ver;
    if (sentry__get_kernel_version(&win_ver)) {
        at_least_one_key_successful = true;

        snprintf(buf, sizeof(buf), "%u.%u.%u.%u", win_ver.major, win_ver.minor,
            win_ver.build, win_ver.ubr);
        sentry_value_set_by_key(
            os, "kernel_version", sentry_value_new_string(buf));
    }

    if (sentry__get_windows_version(&win_ver)) {
        at_least_one_key_successful = true;

        snprintf(buf, sizeof(buf), "%u.%u.%u", win_ver.major, win_ver.minor,
            win_ver.build);
        sentry_value_set_by_key(os, "version", sentry_value_new_string(buf));

        snprintf(buf, sizeof(buf), "%u", win_ver.ubr);
        sentry_value_set_by_key(os, "build", sentry_value_new_string(buf));
    }

    if (at_least_one_key_successful) {
        sentry_value_freeze(os);
        return os;
    }

    sentry_value_decref(os);
    return sentry_value_new_null();
#    endif // defined(SENTRY_PLATFORM_XBOX)
}

#    ifndef SENTRY_UNITTEST
static
#    endif
    void(WINAPI *g_kernel32_GetSystemTimePreciseAsFileTime)(LPFILETIME)
    = NULL;
#    ifndef SENTRY_UNITTEST
static
#    endif
    BOOL(WINAPI *g_kernel32_SetThreadStackGuarantee)(PULONG)
    = NULL;
#    ifndef SENTRY_UNITTEST
static
#    endif
    void(WINAPI *g_kernel32_GetCurrentThreadStackLimits)(PULONG_PTR, PULONG_PTR)
    = NULL;

void
sentry__init_cached_kernel32_functions(void)
{
#    define LOAD_FUNCTION(                                                     \
        module, function_name, function_type, function_pointer, message)       \
        do {                                                                   \
            if (!function_pointer) {                                           \
                function_pointer                                               \
                    = (function_type)GetProcAddress(module, function_name);    \
                if (!function_pointer) {                                       \
                    SENTRY_WARNF(message, GetLastError());                     \
                }                                                              \
            }                                                                  \
        } while (0)

    // Only load kernel32 functions for now, since this function is used in
    // `DllMain`. If we ever load something else we must break out into a
    // separate function that then only gets called from `sentry_init()`.
    HINSTANCE kernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!kernel32) {
        return;
    }
    // Retrieve `GetSystemTimePreciseAsFileTime()` for Windows 8+ targets.
    LOAD_FUNCTION(kernel32, "GetSystemTimePreciseAsFileTime",
        void(WINAPI *)(LPFILETIME), g_kernel32_GetSystemTimePreciseAsFileTime,
        "Couldn't load `GetSystemTimePreciseAsFileTime`. Falling back on "
        "`GetSystemTimeAsFileTime`. (error-code: `%lu`)");

    // `SetThreadStackGuarantee()` is available since Windows XP, but exposing
    // it as pointer allows more controlled tests.
    LOAD_FUNCTION(kernel32, "SetThreadStackGuarantee", BOOL(WINAPI *)(PULONG),
        g_kernel32_SetThreadStackGuarantee,
        "Couldn't load `SetThreadStackGuarantee`: "
        "`sentry_set_thread_stack_guarantee()` won't work. (error-code: "
        "`%lu`)");

    // Retrieve `GetCurrentThreadStackLimits()` for Windows 8+ targets.
    LOAD_FUNCTION(kernel32, "GetCurrentThreadStackLimits",
        void(WINAPI *)(PULONG_PTR, PULONG_PTR),
        g_kernel32_GetCurrentThreadStackLimits,
        "Couldn't load `GetCurrentThreadStackLimits`. Auto-initialization of "
        "the thread stack guarantee won't work. (error-code: `%lu`)");

#    undef LOAD_FUNCTION
}

int
sentry_set_thread_stack_guarantee(uint32_t stack_guarantee_in_bytes)
{
    if (!g_kernel32_SetThreadStackGuarantee) {
        return 0;
    }
    DWORD thread_id = GetThreadId(GetCurrentThread());
    ULONG stack_guarantee = 0;
    if (!g_kernel32_SetThreadStackGuarantee(&stack_guarantee)) {
        SENTRY_ERRORF("`SetThreadStackGuarantee` failed with code `%lu` for "
                      "thread %lu when querying the current guarantee",
            GetLastError(), thread_id);
        return 0;
    }
    if (stack_guarantee != 0) {
        SENTRY_WARNF(
            "`ThreadStackGuarantee` already set to %lu bytes for thread %lu",
            stack_guarantee, thread_id);
        return 0;
    }
    stack_guarantee = stack_guarantee_in_bytes;
    if (!g_kernel32_SetThreadStackGuarantee(&stack_guarantee)) {
        SENTRY_ERRORF("`SetThreadStackGuarantee` failed with code `%lu` for "
                      "thread %lu when applying the guarantee of %lu bytes",
            GetLastError(), thread_id);
        return 0;
    }

    return 1;
}

// Resolves the handler stack size (in KiB) from the `SENTRY_HANDLER_STACK_SIZE`
// environment variable, falling back to the compile-time default. This is
// called from `DllMain` via `sentry__set_default_thread_stack_guarantee`, so
// it must avoid CRT (`getenv`, `strtod`, ...) and only use kernel32 exports.
static size_t
sentry__handler_stack_size_kib(void)
{
    char buf[12];
    DWORD n = GetEnvironmentVariableA(
        "SENTRY_HANDLER_STACK_SIZE", buf, sizeof(buf));
    if (n == 0 || n >= sizeof(buf)) {
        return SENTRY_HANDLER_STACK_SIZE;
    }
    size_t val = 0;
    for (DWORD i = 0; i < n; i++) {
        if (buf[i] < '0' || buf[i] > '9') {
            return SENTRY_HANDLER_STACK_SIZE;
        }
        val = val * 10 + (size_t)(buf[i] - '0');
    }
    return (val > 0 && val <= 0xFFFFFFFFu / 1024) ? val
                                                  : SENTRY_HANDLER_STACK_SIZE;
}

void
sentry__set_default_thread_stack_guarantee(void)
{
    if (!g_kernel32_GetCurrentThreadStackLimits) {
        return;
    }

    const size_t stack_size_kib = sentry__handler_stack_size_kib();
    const unsigned long expected_stack_guarantee
        = (unsigned long)stack_size_kib * 1024;
    DWORD thread_id = GetThreadId(GetCurrentThread());
    ULONG_PTR high = 0;
    ULONG_PTR low = 0;
    g_kernel32_GetCurrentThreadStackLimits(&low, &high);
    size_t thread_stack_reserve = high - low;
    uint32_t expected_stack_reserve
        = expected_stack_guarantee * SENTRY_THREAD_STACK_GUARANTEE_FACTOR;

    if (thread_stack_reserve < expected_stack_reserve) {
        SENTRY_WARNF(
            "Cannot set handler stack guarantee of %zuKiB for thread %lu "
            "(stack reserve: %zuKiB, expected factor: %zux, actual: %.2fx)",
            stack_size_kib, thread_id, thread_stack_reserve / 1024,
            (size_t)SENTRY_THREAD_STACK_GUARANTEE_FACTOR,
            expected_stack_reserve / (double)expected_stack_guarantee);
        return;
    }

#    if defined(SENTRY_THREAD_STACK_GUARANTEE_VERBOSE_LOG)
    if (sentry_set_thread_stack_guarantee(expected_stack_guarantee)) {
        SENTRY_INFOF(
            "ThreadStackGuarantee = %lu bytes for "
            "thread %lu (Stack base = 0x%p, limit = 0x%p, size = %llu)",
            expected_stack_guarantee, thread_id, (void *)high, (void *)low,
            thread_stack_reserve);
    }
#    else
    sentry_set_thread_stack_guarantee(expected_stack_guarantee);
#    endif
}

#    if defined(SENTRY_BUILD_SHARED) && !defined(SENTRY_PLATFORM_XBOX)

BOOL APIENTRY
DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    (void)hModule;
    (void)lpReserved;

    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        sentry__init_cached_kernel32_functions();
        EXPLICIT_FALLTHROUGH;
    case DLL_THREAD_ATTACH:
#        if defined(SENTRY_THREAD_STACK_GUARANTEE_AUTO_INIT)
        sentry__set_default_thread_stack_guarantee();
#        endif
        break;
    default:
        return TRUE;
    }
    return TRUE;
}

#    endif // defined(SENTRY_BUILD_SHARED) &&
           // !defined(SENTRY_PLATFORM_XBOX)

void
sentry__get_system_time(LPFILETIME filetime)
{
    if (g_kernel32_GetSystemTimePreciseAsFileTime) {
        g_kernel32_GetSystemTimePreciseAsFileTime(filetime);
        return;
    }

    GetSystemTimeAsFileTime(filetime);
}

#elif defined(SENTRY_PLATFORM_MACOS)

#    include <sys/sysctl.h>
#    include <sys/utsname.h>

sentry_value_t
sentry__get_os_context(void)
{
    sentry_value_t os = sentry_value_new_object();
    if (sentry_value_is_null(os)) {
        return os;
    }

    sentry_value_set_by_key(os, "name", sentry_value_new_string("macOS"));

    char buf[32];
    size_t buf_len = sizeof(buf);

    if (sysctlbyname("kern.osproductversion", buf, &buf_len, NULL, 0) != 0) {
        goto fail;
    }

    size_t num_dots = 0;
    for (size_t i = 0; i < buf_len; i++) {
        if (buf[i] == '.') {
            num_dots += 1;
        }
    }
    if (num_dots < 2 && buf_len + 3 < sizeof(buf)) {
        strcat(buf, ".0");
    }

    sentry_value_set_by_key(os, "version", sentry_value_new_string(buf));

    buf_len = sizeof(buf);
    if (sysctlbyname("kern.osversion", buf, &buf_len, NULL, 0) != 0) {
        goto fail;
    }

    sentry_value_set_by_key(os, "build", sentry_value_new_string(buf));

    struct utsname uts;
    if (uname(&uts) != 0) {
        goto fail;
    }

    sentry_value_set_by_key(
        os, "kernel_version", sentry_value_new_string(uts.release));

    return os;

fail:

    sentry_value_decref(os);
    return sentry_value_new_null();
}
#elif defined(SENTRY_PLATFORM_UNIX) && !defined(SENTRY_PLATFORM_PS)

#    include <fcntl.h>
#    include <sys/utsname.h>

#    if defined(SENTRY_PLATFORM_LINUX)
#        define OS_RELEASE_MAX_LINE_SIZE 256
#        define OS_RELEASE_MAX_KEY_SIZE 64
#        define OS_RELEASE_MAX_VALUE_SIZE 128

static int
parse_os_release_line(const char *line, char *key, char *value)
{
    const char *equals = strchr(line, '=');
    if (equals == NULL)
        return 1;

    unsigned long key_length = MIN(equals - line, OS_RELEASE_MAX_KEY_SIZE - 1);
    strncpy(key, line, key_length);
    key[key_length] = '\0';

    sentry_slice_t value_slice
        = { .ptr = equals + 1, .len = strlen(equals + 1) };

    // some values are wrapped in double quotes
    if (value_slice.len >= 2 && value_slice.ptr[0] == '\"'
        && value_slice.ptr[value_slice.len - 1] == '\"') {
        value_slice.ptr++;
        value_slice.len -= 2;
    }

    sentry__slice_to_buffer(value_slice, value, OS_RELEASE_MAX_VALUE_SIZE);

    return 0;
}

static void
parse_line_into_object(const char *line, sentry_value_t os_dist)
{
    char value[OS_RELEASE_MAX_VALUE_SIZE];
    char key[OS_RELEASE_MAX_KEY_SIZE];

    if (parse_os_release_line(line, key, value) == 0) {
        if (strcmp(key, "ID") == 0) {
            sentry_value_set_by_key(
                os_dist, "name", sentry_value_new_string(value));
        }

        if (strcmp(key, "VERSION_ID") == 0) {
            sentry_value_set_by_key(
                os_dist, "version", sentry_value_new_string(value));
        }

        if (strcmp(key, "PRETTY_NAME") == 0) {
            sentry_value_set_by_key(
                os_dist, "pretty_name", sentry_value_new_string(value));
        }
    }
}

#        ifndef SENTRY_UNITTEST
static
#        endif
    sentry_value_t
    get_linux_os_release(const char *os_rel_path)
{
    const int fd = open(os_rel_path, O_RDONLY);
    if (fd == -1) {
        return sentry_value_new_null();
    }

    sentry_value_t os_dist = sentry_value_new_object();
    char buffer[OS_RELEASE_MAX_LINE_SIZE];
    ssize_t bytes_read;
    ssize_t buffer_rest = 0;
    const char *line = buffer;
    while ((bytes_read = read(
                fd, buffer + buffer_rest, sizeof(buffer) - buffer_rest - 1))
        > 0) {
        ssize_t buffer_end = buffer_rest + bytes_read;
        buffer[buffer_end] = '\0';

        // extract all lines from the valid buffer-range and parse them
        for (char *p = buffer; *p; ++p) {
            if (*p != '\n') {
                continue;
            }
            *p = '\0';
            parse_line_into_object(line, os_dist);
            line = p + 1;
        }

        if (line < buffer + buffer_end) {
            // move the remaining partial line to the start of the buffer
            buffer_rest = buffer + buffer_end - line;
            memmove(buffer, line, buffer_rest);
        } else {
            // reset buffer_rest: the line-end coincided with the buffer-end
            buffer_rest = 0;
        }
        line = buffer;
    }

    if (bytes_read == -1) {
        // read() failed and we can't assume to have valid data
        sentry_value_decref(os_dist);
        os_dist = sentry_value_new_null();
    } else if (buffer_rest > 0) {
        // the file ended w/o a new-line; we still have a line left to parse
        buffer[buffer_rest] = '\0';
        parse_line_into_object(line, os_dist);
    }

    close(fd);

    return os_dist;
}

#    endif // defined(SENTRY_PLATFORM_LINUX)

sentry_value_t
sentry__get_os_context(void)
{
    sentry_value_t os = sentry_value_new_object();
    if (sentry_value_is_null(os)) {
        return os;
    }

    struct utsname uts;
    if (uname(&uts) != 0) {
        goto fail;
    }

    char *build = uts.release;
    size_t num_dots = 0;
    for (; build[0] != '\0'; build++) {
        char c = build[0];
        if (c == '.') {
            num_dots += 1;
        }
        if (!(c >= '0' && c <= '9') && (c != '.' || num_dots > 2)) {
            break;
        }
    }
    char *build_start = build;
    if (build[0] == '-' || build[0] == '.') {
        build_start++;
    }

    if (build_start[0] != '\0') {
        sentry_value_set_by_key(
            os, "build", sentry_value_new_string(build_start));
    }

    build[0] = '\0';

    sentry_value_set_by_key(os, "name", sentry_value_new_string(uts.sysname));
    sentry_value_set_by_key(
        os, "version", sentry_value_new_string(uts.release));

#    if defined(SENTRY_PLATFORM_LINUX)
    /**
     * The file /etc/os-release takes precedence over /usr/lib/os-release.
     * Applications should check for the former, and exclusively use its data if
     * it exists, and only fall back to /usr/lib/os-release if it is missing.
     * Applications should not read data from both files at the same time.
     *
     * From:
     * https://www.freedesktop.org/software/systemd/man/latest/os-release.html#Description
     */
    sentry_value_t os_dist = get_linux_os_release("/etc/os-release");
    if (sentry_value_is_null(os_dist)) {
        os_dist = get_linux_os_release("/usr/lib/os-release");
        if (sentry_value_is_null(os_dist)) {
            return os;
        }
    }
    sentry_value_set_by_key(
        os, "distribution_name", sentry_value_get_by_key(os_dist, "name"));
    sentry_value_set_by_key(os, "distribution_version",
        sentry_value_get_by_key(os_dist, "version"));
    sentry_value_set_by_key(os, "distribution_pretty_name",
        sentry_value_get_by_key(os_dist, "pretty_name"));
    sentry_value_incref(sentry_value_get_by_key(os_dist, "name"));
    sentry_value_incref(sentry_value_get_by_key(os_dist, "version"));
    sentry_value_incref(sentry_value_get_by_key(os_dist, "pretty_name"));
    sentry_value_decref(os_dist);
#    endif // defined(SENTRY_PLATFORM_LINUX)

    return os;

fail:

    sentry_value_decref(os);
    return sentry_value_new_null();
}

#elif defined(SENTRY_PLATFORM_NX) || defined(SENTRY_PLATFORM_PS)

// sentry__get_os_context() is defined in a downstream SDK.

#else

sentry_value_t
sentry__get_os_context(void)
{
    return sentry_value_new_null();
}

#endif

sentry_value_t
sentry__build_registers(const sentry_ucontext_t *uctx)
{
    sentry_value_t registers = sentry_value_new_object();
    if (!uctx) {
        return registers;
    }
#if defined(SENTRY_PLATFORM_LINUX) || defined(SENTRY_PLATFORM_DARWIN)
    if (!uctx->user_context) {
        return registers;
    }
#    if defined(SENTRY_PLATFORM_DARWIN)
    if (!uctx->user_context->uc_mcontext) {
        return registers;
    }
#    endif
#elif defined(SENTRY_PLATFORM_WINDOWS)
    if (!uctx->exception_ptrs.ContextRecord) {
        return registers;
    }
#endif

#if defined(SENTRY_PLATFORM_LINUX)

#    if defined(__x86_64__) || defined(__i386__) || defined(__arm__)
    // just assume the ctx is a bunch of uintpr_t, and index that directly
    uintptr_t *ctx = (uintptr_t *)&uctx->user_context->uc_mcontext;
#    endif

#    define SET_REG(name, num)                                                 \
        sentry_value_set_by_key(registers, name,                               \
            sentry__value_new_addr((uint64_t)(size_t)ctx[num]));

#    if defined(__x86_64__)

    SET_REG("r8", 0);
    SET_REG("r9", 1);
    SET_REG("r10", 2);
    SET_REG("r11", 3);
    SET_REG("r12", 4);
    SET_REG("r13", 5);
    SET_REG("r14", 6);
    SET_REG("r15", 7);
    SET_REG("rdi", 8);
    SET_REG("rsi", 9);
    SET_REG("rbp", 10);
    SET_REG("rbx", 11);
    SET_REG("rdx", 12);
    SET_REG("rax", 13);
    SET_REG("rcx", 14);
    SET_REG("rsp", 15);
    SET_REG("rip", 16);

#    elif defined(__i386__)

    // gs, fs, es, ds
    SET_REG("edi", 4);
    SET_REG("esi", 5);
    SET_REG("ebp", 6);
    SET_REG("esp", 7);
    SET_REG("ebx", 8);
    SET_REG("edx", 9);
    SET_REG("ecx", 10);
    SET_REG("eax", 11);
    SET_REG("eip", 14);
    SET_REG("eflags", 16);

#    elif defined(__aarch64__)

    // Use struct field access instead of raw pointer indexing because
    // struct sigcontext has fault_address before regs[31] on aarch64.
    for (int i = 0; i < 29; i++) {
        char name[4];
        snprintf(name, sizeof(name), "x%d", i);
        sentry_value_set_by_key(registers, name,
            sentry__value_new_addr(uctx->user_context->uc_mcontext.regs[i]));
    }
    sentry_value_set_by_key(registers, "fp",
        sentry__value_new_addr(uctx->user_context->uc_mcontext.regs[29]));
    sentry_value_set_by_key(registers, "lr",
        sentry__value_new_addr(uctx->user_context->uc_mcontext.regs[30]));
    sentry_value_set_by_key(registers, "sp",
        sentry__value_new_addr(uctx->user_context->uc_mcontext.sp));
    sentry_value_set_by_key(registers, "pc",
        sentry__value_new_addr(uctx->user_context->uc_mcontext.pc));

#    elif defined(__arm__)

    // trap_no, _error_code, oldmask
    SET_REG("r0", 3);
    SET_REG("r1", 4);
    SET_REG("r2", 5);
    SET_REG("r3", 6);
    SET_REG("r4", 7);
    SET_REG("r5", 8);
    SET_REG("r6", 9);
    SET_REG("r7", 10);
    SET_REG("r8", 11);
    SET_REG("r9", 12);
    SET_REG("r10", 13);
    SET_REG("fp", 14);
    SET_REG("ip", 15);
    SET_REG("sp", 16);
    SET_REG("lr", 17);
    SET_REG("pc", 18);
    sentry_value_set_by_key(registers, "cpsr",
        sentry__value_new_addr(uctx->user_context->uc_mcontext.arm_cpsr));

#    endif

#    undef SET_REG

#elif defined(SENTRY_PLATFORM_DARWIN)

#    define SET_REG(name, prop)                                                \
        sentry_value_set_by_key(registers, name,                               \
            sentry__value_new_addr((uint64_t)(size_t)thread_state->prop));

#    if defined(__x86_64__)

    _STRUCT_X86_THREAD_STATE64 *thread_state
        = &uctx->user_context->uc_mcontext->__ss;

    SET_REG("rax", __rax);
    SET_REG("rbx", __rbx);
    SET_REG("rcx", __rcx);
    SET_REG("rdx", __rdx);
    SET_REG("rdi", __rdi);
    SET_REG("rsi", __rsi);
    SET_REG("rbp", __rbp);
    SET_REG("rsp", __rsp);
    SET_REG("r8", __r8);
    SET_REG("r9", __r9);
    SET_REG("r10", __r10);
    SET_REG("r11", __r11);
    SET_REG("r12", __r12);
    SET_REG("r13", __r13);
    SET_REG("r14", __r14);
    SET_REG("r15", __r15);
    SET_REG("rip", __rip);

#    elif defined(__arm64__)

    _STRUCT_ARM_THREAD_STATE64 *thread_state
        = &uctx->user_context->uc_mcontext->__ss;

    SET_REG("x0", __x[0]);
    SET_REG("x1", __x[1]);
    SET_REG("x2", __x[2]);
    SET_REG("x3", __x[3]);
    SET_REG("x4", __x[4]);
    SET_REG("x5", __x[5]);
    SET_REG("x6", __x[6]);
    SET_REG("x7", __x[7]);
    SET_REG("x8", __x[8]);
    SET_REG("x9", __x[9]);
    SET_REG("x10", __x[10]);
    SET_REG("x11", __x[11]);
    SET_REG("x12", __x[12]);
    SET_REG("x13", __x[13]);
    SET_REG("x14", __x[14]);
    SET_REG("x15", __x[15]);
    SET_REG("x16", __x[16]);
    SET_REG("x17", __x[17]);
    SET_REG("x18", __x[18]);
    SET_REG("x19", __x[19]);
    SET_REG("x20", __x[20]);
    SET_REG("x21", __x[21]);
    SET_REG("x22", __x[22]);
    SET_REG("x23", __x[23]);
    SET_REG("x24", __x[24]);
    SET_REG("x25", __x[25]);
    SET_REG("x26", __x[26]);
    SET_REG("x27", __x[27]);
    SET_REG("x28", __x[28]);
#        if __DARWIN_OPAQUE_ARM_THREAD_STATE64
    sentry_value_set_by_key(registers, "fp",
        sentry__value_new_addr(
            (uint64_t)__darwin_arm_thread_state64_get_fp(*thread_state)));
    sentry_value_set_by_key(registers, "lr",
        sentry__value_new_addr(
            (uint64_t)__darwin_arm_thread_state64_get_lr(*thread_state)));
    sentry_value_set_by_key(registers, "sp",
        sentry__value_new_addr(
            (uint64_t)__darwin_arm_thread_state64_get_sp(*thread_state)));
    sentry_value_set_by_key(registers, "pc",
        sentry__value_new_addr(
            (uint64_t)__darwin_arm_thread_state64_get_pc(*thread_state)));
#        else
    SET_REG("fp", __fp);
    SET_REG("lr", __lr);
    SET_REG("sp", __sp);
    SET_REG("pc", __pc);
#        endif

#    elif defined(__arm__)

    _STRUCT_ARM_THREAD_STATE *thread_state
        = &uctx->user_context->uc_mcontext->__ss;

    SET_REG("r0", __r[0]);
    SET_REG("r1", __r[1]);
    SET_REG("r2", __r[2]);
    SET_REG("r3", __r[3]);
    SET_REG("r4", __r[4]);
    SET_REG("r5", __r[5]);
    SET_REG("r6", __r[6]);
    SET_REG("r7", __r[7]);
    SET_REG("r8", __r[8]);
    SET_REG("r9", __r[9]);
    SET_REG("r10", __r[10]);
    SET_REG("fp", __r[11]);
    SET_REG("ip", __r[12]);
    SET_REG("sp", __sp);
    SET_REG("lr", __lr);
    SET_REG("pc", __pc);

#    endif

#    undef SET_REG

#elif defined(SENTRY_PLATFORM_WINDOWS)
    PCONTEXT ctx = uctx->exception_ptrs.ContextRecord;

#    define SET_REG(name, prop)                                                \
        sentry_value_set_by_key(registers, name,                               \
            sentry__value_new_addr((uint64_t)(size_t)ctx->prop))

#    if defined(_M_AMD64)

    if ((ctx->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {
        SET_REG("rax", Rax);
        SET_REG("rcx", Rcx);
        SET_REG("rdx", Rdx);
        SET_REG("rbx", Rbx);
        SET_REG("rbp", Rbp);
        SET_REG("rsi", Rsi);
        SET_REG("rdi", Rdi);
        SET_REG("r8", R8);
        SET_REG("r9", R9);
        SET_REG("r10", R10);
        SET_REG("r11", R11);
        SET_REG("r12", R12);
        SET_REG("r13", R13);
        SET_REG("r14", R14);
        SET_REG("r15", R15);
    }

    if ((ctx->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {
        SET_REG("rsp", Rsp);
        SET_REG("rip", Rip);
    }

#    elif defined(_M_IX86)

    if ((ctx->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {
        SET_REG("edi", Edi);
        SET_REG("esi", Esi);
        SET_REG("ebx", Ebx);
        SET_REG("edx", Edx);
        SET_REG("ecx", Ecx);
        SET_REG("eax", Eax);
    }

    if ((ctx->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {
        SET_REG("ebp", Ebp);
        SET_REG("eip", Eip);
        SET_REG("eflags", EFlags);
        SET_REG("esp", Esp);
    }

#    elif defined(_M_ARM64)

    if ((ctx->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {
        SET_REG("x0", X0);
        SET_REG("x1", X1);
        SET_REG("x2", X2);
        SET_REG("x3", X3);
        SET_REG("x4", X4);
        SET_REG("x5", X5);
        SET_REG("x6", X6);
        SET_REG("x7", X7);
        SET_REG("x8", X8);
        SET_REG("x9", X9);
        SET_REG("x10", X10);
        SET_REG("x11", X11);
        SET_REG("x12", X12);
        SET_REG("x13", X13);
        SET_REG("x14", X14);
        SET_REG("x15", X15);
        SET_REG("x16", X16);
        SET_REG("x17", X17);
        SET_REG("x18", X18);
        SET_REG("x19", X19);
        SET_REG("x20", X20);
        SET_REG("x21", X21);
        SET_REG("x22", X22);
        SET_REG("x23", X23);
        SET_REG("x24", X24);
        SET_REG("x25", X25);
        SET_REG("x26", X26);
        SET_REG("x27", X27);
        SET_REG("x28", X28);
    }

    if ((ctx->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {
        SET_REG("fp", Fp);
        SET_REG("lr", Lr);
        SET_REG("sp", Sp);
        SET_REG("pc", Pc);
    }

#    elif defined(_M_ARM)

    if ((ctx->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {
        SET_REG("fp", R11);
    }

    if ((ctx->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {
        SET_REG("sp", Sp);
        SET_REG("pc", Pc);
    }

#    endif

#    undef SET_REG

#endif

    return registers;
}
