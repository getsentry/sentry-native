#include "../sentry_string.h"

char *
sentry__string_from_wstr(wchar_t *s)
{
    int len = WideCharToMultiByte(CP_UTF8, 0, s, -1, NULL, 0, NULL, NULL);
    char *rv = sentry_malloc(len);
    if (rv) {
        WideCharToMultiByte(CP_UTF8, 0, s, -1, rv, len, NULL, NULL);
    }
    return rv;
}
