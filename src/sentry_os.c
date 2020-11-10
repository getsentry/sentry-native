#include "sentry_os.h"
#include "sentry_string.h"

#include <winver.h>

char *
sentry__os_full_version(void)
{
    char *out = NULL;

    void *ffibuf = NULL;

    DWORD size = GetFileVersionInfoSizeW(L"kernel32.dll", NULL);
    if (!size) {
        goto exit;
    }

    ffibuf = sentry_malloc(size);
    if (!GetFileVersionInfoW(L"kernel32.dll", 0, size, ffibuf)) {
        goto exit;
    }

    VS_FIXEDFILEINFO *ffi;
    UINT ffi_size;
    if (!VerQueryValueW(ffibuf, L"\\", &ffi, &ffi_size)) {
        goto exit;
    }
    ffi->dwFileFlags &= ffi->dwFileFlagsMask;

    const char *os = ((ffi->dwFileOS & VOS_NT_WINDOWS32) == VOS_NT_WINDOWS32)
        ? "Windows NT"
        : "Unknown";
    char buf[100];
    snprintf(buf, sizeof(buf), "%s %u.%u.%u.%lu", os,
        ffi->dwFileVersionMS >> 16, ffi->dwFileVersionMS & 0xffff,
        ffi->dwFileVersionLS >> 16, ffi->dwFileVersionLS & 0xffff);
    out = sentry__string_clone(buf);

exit:
    sentry_free(ffibuf);
    return out;
}
