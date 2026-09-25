#include "sentry.h"
#include <stdlib.h>

static sentry_value_t
before_send_log(sentry_value_t log, void *user_data)
{
    (void)user_data;

    sentry_value_t attributes = sentry_value_get_by_key(log, "attributes");
    sentry_value_remove_by_key(attributes, "auth.token");
    return log;
}

int
main(void)
{
    sentry_options_t *options = sentry_options_new();
    // sentry_options_set_dsn(options, "https://KEY@oORG.ingest.sentry.io/PRJ");
    sentry_options_set_debug(options, 1);
    sentry_options_set_before_send_log(options, before_send_log, NULL);

    if (sentry_init(options) != 0) {
        return EXIT_FAILURE;
    }

    sentry_log(SENTRY_LEVEL_INFO, "Sending request", sentry_value_new_null());

    sentry_value_t attributes = sentry_value_new_object();
    sentry_value_set_by_key(attributes, "retry.count",
        sentry_value_new_attribute(sentry_value_new_int32(1), NULL));
    sentry_value_set_by_key(attributes, "auth.token",
        sentry_value_new_attribute(
            sentry_value_new_string("secret-token"), NULL));
    sentry_log(SENTRY_LEVEL_WARNING, "Retrying request", attributes);

    sentry_close();
}
