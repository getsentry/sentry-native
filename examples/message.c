#include "sentry.h"
#include <stdlib.h>

static sentry_value_t
before_send(sentry_value_t event, sentry_hint_t *hint, void *user_data)
{
    (void)hint;
    (void)user_data;

    sentry_value_remove_by_key(
        sentry_value_get_by_key(event, "extra"), "access_token");
    return event;
}

int
main(void)
{
    sentry_options_t *options = sentry_options_new();
    // sentry_options_set_dsn(options, "https://KEY@oORG.ingest.sentry.io/PRJ");
    sentry_options_set_debug(options, 1);
    sentry_options_set_before_send(options, before_send, NULL);

    if (sentry_init(options) != 0) {
        return EXIT_FAILURE;
    }

    sentry_set_extra("access_token", sentry_value_new_string("secret-token"));

    sentry_value_t event = sentry_value_new_message_event(
        SENTRY_LEVEL_INFO, "example", "Hello from sentry-native");
    sentry_capture_event(event);

    sentry_close();
}
