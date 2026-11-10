#include "sentry.h"
#include <stdlib.h>

static sentry_value_t
on_crash(const sentry_ucontext_t *uctx, sentry_value_t event,
    sentry_hint_t *hint, void *user_data)
{
    (void)uctx;
    // send the crash to the DSN passed as user data, if any
    sentry_hint_set_dsn(hint, user_data);
    return event;
}

int
main(void)
{
    sentry_options_t *options = sentry_options_new();
    // sentry_options_set_dsn(options, "https://KEY@oORG.ingest.sentry.io/PRJ");
    sentry_options_set_debug(options, 1);
    sentry_options_set_on_crash(options, on_crash, getenv("SENTRY_CRASH_DSN"));

    if (sentry_init(options) != 0) {
        return EXIT_FAILURE;
    }

    sentry_crash();
}
