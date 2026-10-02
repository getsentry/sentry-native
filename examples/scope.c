#include "sentry.h"
#include <stdlib.h>

int
main(void)
{
    sentry_options_t *options = sentry_options_new();
    // sentry_options_set_dsn(options, "https://KEY@oORG.ingest.sentry.io/PRJ");
    sentry_options_set_debug(options, 1);

    if (sentry_init(options) != 0) {
        return EXIT_FAILURE;
    }

    sentry_set_tag("example.scope", "global");

    sentry_scope_t *scope = sentry_local_scope_new();
    sentry_scope_set_tag(scope, "example.scope", "local");
    sentry_scope_set_user(
        scope, sentry_value_new_user("123", "user", NULL, NULL));

    sentry_value_t event = sentry_value_new_message_event(
        SENTRY_LEVEL_INFO, "example", "Captured with a local scope");
    sentry_scope_capture_event(scope, event, NULL);

    sentry_capture_event(sentry_value_new_message_event(
        SENTRY_LEVEL_INFO, "example", "Captured with the global scope"));

    sentry_close();
}
