#include "sentry_os.h"
#include "sentry_string.h"
#include "sentry_utils.h"

#ifdef SENTRY_PLATFORM_WINDOWS

#    include <winver.h>
#    define CURRENT_VERSION "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"

void *
sentry__try_file_version(const LPCWSTR filename)
{
    const DWORD size = GetFileVersionInfoSizeW(filename, NULL);
    if (!size) {
        return NULL;
    }

    void *ffibuf = sentry_malloc(size);
    if (!GetFileVersionInfoW(filename, 0, size, ffibuf)) {
        sentry_free(ffibuf);
        return NULL;
    }
    return ffibuf;
}

int
sentry__get_kernel_version(windows_version_t *win_ver)
{
    void *ffibuf = sentry__try_file_version(L"ntoskrnl.exe");
    if (!ffibuf) {
        ffibuf = sentry__try_file_version(L"kernel32.dll");
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
    win_ver->build = (uint32_t)sentry__strtod_c(buf, NULL);

    buf_size = sizeof(uint32_t);
    if (RegGetValueA(HKEY_LOCAL_MACHINE, CURRENT_VERSION, "UBR",
            RRF_RT_REG_DWORD, NULL, &reg_version, &buf_size)
        != ERROR_SUCCESS) {
        return 0;
    }
    win_ver->ubr = reg_version;

    return 1;
}

sentry_value_t
sentry__get_os_context(void)
{
    const sentry_value_t os = sentry_value_new_object();
    if (sentry_value_is_null(os)) {
        return os;
    }
    sentry_value_set_by_key(os, "name", sentry_value_new_string("Windows"));

    bool at_least_one_key_successful = false;
    char buf[32];
    windows_version_t win_ver;
    if (sentry__get_kernel_version(&win_ver)) {
        at_least_one_key_successful = true;

        snprintf(buf, sizeof(buf), "%u.%u.%u.%lu", win_ver.major, win_ver.minor,
            win_ver.build, win_ver.ubr);
        sentry_value_set_by_key(
            os, "kernel_version", sentry_value_new_string(buf));
    }

    if (sentry__get_windows_version(&win_ver)) {
        at_least_one_key_successful = true;

        snprintf(buf, sizeof(buf), "%u.%u.%u", win_ver.major, win_ver.minor,
            win_ver.build);
        sentry_value_set_by_key(os, "version", sentry_value_new_string(buf));

        snprintf(buf, sizeof(buf), "%lu", win_ver.ubr);
        sentry_value_set_by_key(os, "build", sentry_value_new_string(buf));
    }

    if (at_least_one_key_successful) {
        sentry_value_freeze(os);
        return os;
    }

    sentry_value_decref(os);
    return sentry_value_new_null();
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
#elif defined(SENTRY_PLATFORM_UNIX)

#    include <fcntl.h>
#    include <string.h>
#    include <sys/utsname.h>
#    include <unistd.h>

#    define OS_RELEASE_MAX_LINE_SIZE 128
#    define OS_RELEASE_MAX_KEY_SIZE 64
#    define OS_RELEASE_MAX_VALUE_SIZE 128

static int
parse_os_release_line(const char *line, char *key, char *value)
{
    const char *equals = strchr(line, '=');
    if (equals == NULL)
        return 1;

    unsigned long key_length = equals - line;
    strncpy(key, line, key_length);
    key[key_length] = 0;

    unsigned long value_length = strlen(equals + 1);
    strncpy(value, equals + 1, value_length);
    value[value_length] = 0;

    // some values are wrapped in double quotes
    if (value[0] == '\"') {
        value[value_length - 1] = 0;
        memmove(value, value + 1, value_length);
    }

    return 0;
}

#    ifndef SENTRY_UNITTEST
static
#    endif
    sentry_value_t
    get_linux_os_release(const char *os_rel_path)
{
    sentry_value_t os_dist = sentry_value_new_object();
    const int fd = open(os_rel_path, O_RDONLY);
    if (fd == -1) {
        return sentry_value_new_null();
    }

    char buffer[OS_RELEASE_MAX_LINE_SIZE];
    ssize_t bytes_read;
    ssize_t buffer_rest = 0;
    const char *line = buffer;
    while ((bytes_read = read(
                fd, buffer + buffer_rest, sizeof(buffer) - buffer_rest - 1))
        > 0) {
        buffer[bytes_read] = 0;

        for (char *p = buffer; *p; ++p) {
            if (*p != '\n')
                continue;

            char value[OS_RELEASE_MAX_VALUE_SIZE];
            char key[OS_RELEASE_MAX_KEY_SIZE];
            *p = '\0';

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
            line = p + 1;
        }

        // Handle any partial line left at the end of the buffer
        if (line < buffer + bytes_read) {
            buffer_rest = buffer + bytes_read - line;
            memmove(buffer, line, buffer_rest);
            line = buffer;
        }
    }

    if (bytes_read == -1) {
        close(fd);
        return sentry_value_new_null();
    }

    close(fd);

    return os_dist;
}

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

    /**
     * The file /etc/os-release takes precedence over /usr/lib/os-release.
     * Applications should check for the former, and exclusively use its data if
     * it exists, and only fall back to /usr/lib/os-release if it is missing.
     * Applications should not read data from both files at the same time.
     *
     * From: https://www.freedesktop.org/software/systemd/man/latest/os-release.html#Description
     */
    sentry_value_t os_dist = get_linux_os_release("/etc/os-release");
    if (sentry_value_is_null(os_dist)) {
        os_dist = get_linux_os_release("/usr/lib/os-release");
        if (sentry_value_is_null(os_dist)) {
            sentry_value_set_by_key(os, "distribution", os_dist);
        }
    } else {
        sentry_value_set_by_key(os, "distribution", os_dist);
    }

    return os;

fail:

    sentry_value_decref(os);
    return sentry_value_new_null();
}

#else

sentry_value_t
sentry__get_os_context(void)
{
    return sentry_value_new_null();
}

#endif
