#include "sentry.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static sentry_value_t
before_send_feedback(sentry_value_t event, sentry_hint_t *hint, void *user_data)
{
    (void)hint;
    (void)user_data;

    sentry_value_t feedback = sentry_value_get_by_key(
        sentry_value_get_by_key(event, "contexts"), "feedback");
    const char *email = sentry_value_as_string(
        sentry_value_get_by_key(feedback, "contact_email"));
    if (strcmp(email, "spam@example.com") == 0) {
        sentry_value_decref(event);
        return sentry_value_new_null();
    }

    return event;
}

int
main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr,
            "Usage: %s <envelope> <message> [email] [name] [attachment ...]\n",
            argv[0]);
        return EXIT_FAILURE;
    }

    const char *path = argv[1];
    sentry_envelope_t *envelope = sentry_envelope_read_from_file(path);
    sentry_uuid_t event_id = sentry_envelope_get_event_id(envelope);
    sentry_envelope_free(envelope);
    if (sentry_uuid_is_nil(&event_id)) {
        fprintf(stderr, "Error: invalid event %s\n", path);
        return EXIT_FAILURE;
    }

    sentry_options_t *options = sentry_options_new();
    sentry_options_set_debug(options, true);
    sentry_options_set_before_send_feedback(
        options, before_send_feedback, NULL);

    if (sentry_init(options) != 0) {
        return EXIT_FAILURE;
    }

    const char *message = argv[2];
    const char *email = argc > 3 ? argv[3] : NULL;
    const char *name = argc > 4 ? argv[4] : NULL;
    sentry_value_t feedback
        = sentry_value_new_feedback(message, email, name, &event_id);
    sentry_hint_t *hint = sentry_hint_new();
    for (int i = 5; i < argc; i++) {
        sentry_hint_attach_file(hint, argv[i]);
    }
    sentry_capture_feedback_with_hint(feedback, hint);

    sentry_close();
}
