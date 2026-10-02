#include "sentry.h"
#include <stdlib.h>

static sentry_value_t
before_send_feedback(
    sentry_value_t feedback, sentry_hint_t *hint, void *user_data)
{
    (void)user_data;

    static const char diagnostics[] = "example=feedback\n";
    sentry_hint_attach_bytes(
        hint, diagnostics, sizeof(diagnostics) - 1, "diagnostics.txt");
    return feedback;
}

int
main(void)
{
    sentry_options_t *options = sentry_options_new();
    // sentry_options_set_dsn(options, "https://KEY@oORG.ingest.sentry.io/PRJ");
    sentry_options_set_debug(options, 1);
    sentry_options_set_before_send_feedback(
        options, before_send_feedback, NULL);

    if (sentry_init(options) != 0) {
        return EXIT_FAILURE;
    }

    sentry_uuid_t event_id
        = sentry_capture_event(sentry_value_new_message_event(
            SENTRY_LEVEL_INFO, "example", "Feedback example event"));

    sentry_capture_feedback(
        sentry_value_new_feedback("The app is working well.",
            "user@example.com", "Example User", &event_id));

    sentry_close();
}
