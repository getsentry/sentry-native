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

    sentry_set_user(sentry_value_new_user("123", "user", NULL, NULL));
    sentry_set_tag("example", "enrich");
    sentry_set_transaction("resource.load");

    sentry_value_t resource = sentry_value_new_object();
    sentry_value_set_by_key(
        resource, "type", sentry_value_new_string("configuration"));
    sentry_value_set_by_key(resource, "cached", sentry_value_new_bool(0));
    sentry_set_context("resource", resource);

    sentry_value_t event = sentry_value_new_message_event(
        SENTRY_LEVEL_INFO, "example", "Resource loaded");
    sentry_capture_event(event);

    sentry_close();
}
