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

int
main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <envelope>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *envelope = fopen(argv[1], "r");
    if (!envelope) {
        fprintf(stderr, "ERROR: %s (%s)\n", argv[1], strerror(errno));
        return EXIT_FAILURE;
    }

    char header[768];
    if (!fgets(header, sizeof(header), envelope)) {
        fprintf(stderr, "ERROR: %s (%s)\n", argv[1], strerror(errno));
        fclose(envelope);
        return EXIT_FAILURE;
    }
    fclose(envelope);

    char dsn[513];
    char event_id[37];
    int parsed = sscanf(header,
        "{\"dsn\":\"%512[^\"]\",\"event_id\":\"%36[^\"]\"}", dsn, event_id);
    if (parsed != 2) {
        fprintf(stderr, "ERROR: Failed to parse header\n");
    }

    sentry_options_t *options = sentry_options_new();
    sentry_options_set_backend(options, NULL);
    sentry_options_set_dsn(options, dsn);
    sentry_options_set_debug(options, true);
    sentry_init(options);

    sentry_uuid_t uuid = sentry_uuid_from_string(event_id);
    sentry_value_t feedback = sentry_value_new_feedback(
        "some-message", "some-email", "some-name", &uuid);

    sentry_capture_feedback(feedback);

    sentry_close();
}
