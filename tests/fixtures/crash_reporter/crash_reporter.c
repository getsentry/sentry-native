#ifdef _WIN32
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    define NOMINMAX
#    define _CRT_SECURE_NO_WARNINGS
#endif

#include "sentry.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#    include <windows.h>

static char *
wstr_to_utf8(const wchar_t *wstr)
{
    if (!wstr) {
        return NULL;
    }
    const int len
        = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (len <= 0) {
        return NULL;
    }
    char *utf8 = malloc(len);
    if (!utf8) {
        return NULL;
    }
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, utf8, len, NULL, NULL);
    return utf8;
}

// Use a wide-string CLI interface for ACP independence wrt the console and
// convert the wide strings back to narrow UTF-8 when printing to stderr, so
// we don't depend on a locale configuration in the CRT.
int
wmain(int argc, wchar_t *argv[])
{
    if (argc != 2) {
        char *argv0_utf8 = wstr_to_utf8(argv[0]);
        fprintf(stderr, "Usage: %s <envelope>\n",
            argv0_utf8 ? argv0_utf8 : "crash_reporter");
        free(argv0_utf8);
        return EXIT_FAILURE;
    }

    wchar_t *path = argv[1];
    sentry_envelope_t *envelope = sentry_envelope_read_from_filew(path);
    if (!envelope) {
        char *path_utf8 = wstr_to_utf8(path);
        char *error_utf8 = wstr_to_utf8(_wcserror(errno));
        fprintf(stderr, "ERROR: %s (%s)\n", path_utf8 ? path_utf8 : "<path>",
            error_utf8 ? error_utf8 : strerror(errno));
        free(path_utf8);
        free(error_utf8);
        return EXIT_FAILURE;
    }
#else
int
main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <envelope>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *path = argv[1];
    sentry_envelope_t *envelope = sentry_envelope_read_from_file(path);
    if (!envelope) {
        fprintf(stderr, "ERROR: %s (%s)\n", path, strerror(errno));
        return EXIT_FAILURE;
    }
#endif

    sentry_value_t dsn = sentry_envelope_get_header(envelope, "dsn");
    sentry_value_t event_id = sentry_envelope_get_header(envelope, "event_id");

    sentry_options_t *options = sentry_options_new();
    sentry_options_set_backend(options, NULL);
    sentry_options_set_dsn(options, sentry_value_as_string(dsn));
    sentry_options_set_debug(options, true);
    sentry_init(options);

    sentry_uuid_t uuid
        = sentry_uuid_from_string(sentry_value_as_string(event_id));
    sentry_value_t feedback = sentry_value_new_feedback(
        "some-message", "some-email", "some-name", &uuid);

    sentry_capture_envelope(envelope);
    sentry_capture_feedback(feedback);

    sentry_close();

#ifdef _WIN32
    _wremove(path);
#else
    remove(path);
#endif
}
