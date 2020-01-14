#include "sentry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(void)
{
    sentry_options_t *options = sentry_options_new();

    sentry_options_set_dsn(
        options, "http://a9c8c48ae72643a0affaeb4b15548768@localhost:8000/1");
    sentry_options_set_environment(options, "Production");
    sentry_options_set_release(options, "5fd7a6cd");
    sentry_options_set_debug(options, 1);

    sentry_init(options);

    sentry_set_transaction("tran");
    sentry_set_level(SENTRY_LEVEL_WARNING);
    sentry_set_extra("extra stuff", sentry_value_new_string("some value"));
    sentry_set_tag("expected-tag", "some value");
    sentry_set_tag("not-expected-tag", "some value");
    sentry_remove_tag("not-expected-tag");
    sentry_set_fingerprint("foo", "bar", NULL);

    sentry_value_t user = sentry_value_new_object();
    sentry_value_set_by_key(user, "id", sentry_value_new_int32(42));
    sentry_value_set_by_key(
        user, "username", sentry_value_new_string("some_name"));
    sentry_set_user(user);

    // memset((char *)0x0, 1, 100);

    sentry_value_t event = sentry_value_new_event();
    sentry_value_set_by_key(
        event, "message", sentry_value_new_string("Hello World!"));
    sentry_capture_event(event);

    // make sure everything flushes
    sentry_shutdown();
}
