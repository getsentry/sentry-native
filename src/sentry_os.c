#include "sentry_os.h"
#include "sentry_path.h"
#include "sentry_slice.h"
#include "sentry_string.h"
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
    if (!g_sigabrt_installed) {
        g_previous_sigabrt_handler = signal(SIGABRT, handle_sigabrt);
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

    buf_size = sizeof(uint32_t);
    if (RegGetValueA(HKEY_LOCAL_MACHINE, CURRENT_VERSION, "UBR",
            RRF_RT_REG_DWORD, NULL, &reg_version, &buf_size)
        != ERROR_SUCCESS) {
        return 0;
    }
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

static char *
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
    return sentry__stringbuilder_into_string(&sb);
}

static char *
read_wine_file(const char *path)
{
    sentry_path_t *file_path = sentry__path_from_str(path);
    char *contents
        = file_path ? sentry__path_read_to_buffer(file_path, NULL) : NULL;
    sentry__path_free(file_path);
    return contents;
}

static char *
get_proton_version(bool *is_proton)
{
    *is_proton = false;
    const char *compat_path = getenv("STEAM_COMPAT_DATA_PATH");
    if (!compat_path || !compat_path[0]) {
        return NULL;
    }

    char *config_path = make_wine_path(compat_path, "/config_info");
    char *config = config_path ? read_wine_file(config_path) : NULL;
    sentry_free(config_path);
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

    char *proton_path = make_wine_path(proton_root, "/proton");
    sentry_path_t *proton_file = sentry__path_from_str(proton_path);
    *is_proton = proton_file && sentry__path_is_file(proton_file);
    sentry__path_free(proton_file);
    sentry_free(proton_path);

    char *version_path = make_wine_path(proton_root, "/version");
    sentry_free(proton_root);
    char *version_file = version_path ? read_wine_file(version_path) : NULL;
    sentry_free(version_path);
    if (!version_file) {
        return NULL;
    }

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

sentry_value_t
sentry__make_wine_context(sentry__wine_get_version_t wine_get_version,
    const char *proton_version, bool is_proton)
{
    if (!wine_get_version) {
        return sentry_value_new_null();
    }

    const char *runtime_name = "Wine";
    const char *runtime_version = wine_get_version();
    if (!runtime_version || !runtime_version[0]) {
        return sentry_value_new_null();
    }

    if (proton_version && proton_version[0]) {
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

    sentry_value_t context = sentry_value_new_object();
    if (sentry_value_is_null(context)) {
        return context;
    }
    sentry_value_set_by_key(
        context, "type", sentry_value_new_string("runtime"));
    sentry_value_set_by_key(
        context, "name", sentry_value_new_string(runtime_name));
    if (runtime_version[0]) {
        sentry_value_set_by_key(
            context, "version", sentry_value_new_string(runtime_version));
    }
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
    bool is_proton = false;
    char *proton_version = get_proton_version(&is_proton);
    sentry_value_t context = sentry__make_wine_context(
        wine_get_version, proton_version, is_proton);
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
    if (value_slice.ptr[0] == '\"') {
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
