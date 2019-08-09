#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sentry.h"

#ifdef _WIN32
const char *handler_path = "bin/Release/crashpad_handler.exe";
#else
const char *handler_path = "bin/Release/crashpad_handler";
#endif

int main(void) {
    sentry_options_t *options = sentry_options_new();

    sentry_options_set_dsn(options, getenv("SENTRY_DSN"));
    sentry_options_set_handler_path(options, handler_path);
    sentry_options_set_environment(options, "Production");
    sentry_options_set_release(options, "5fd7a6cd");
    sentry_options_set_database_path(options, "sentrypad-db");
    sentry_options_set_debug(options, 1);
    sentry_options_add_attachment(options, "example", "../example.c");

    sentry_init(options);

    sentry_set_transaction("tran");
    sentry_set_level(SENTRY_LEVEL_WARNING);
    sentry_set_extra("extra stuff", sentry_value_new_string("some value"));
    sentry_set_tag("expected-tag", "some value");
    sentry_set_tag("not-expected-tag", "some value");
    sentry_remove_tag("not-expected-tag");
    sentry_set_fingerprint("foo", "bar", NULL);

    sentry_value_t default_crumb =
        sentry_breadcrumb_value_new(0, "default level is info");
    sentry_add_breadcrumb(default_crumb);

    sentry_value_t debug_crumb = sentry_breadcrumb_value_new("http", "debug crumb");
    sentry_value_set_key(debug_crumb, "category", sentry_value_new_string("example!"));
    sentry_value_set_key(debug_crumb, "level", sentry_value_new_string("debug"));
    sentry_add_breadcrumb(debug_crumb);

    for (size_t i = 0; i < 101; i++) {
        char buffer[4];
        sprintf(buffer, "%zu", i);
        sentry_add_breadcrumb(sentry_breadcrumb_value_new(0, buffer));
    }

    sentry_value_t user = sentry_value_new_object();
    sentry_value_set_key(user, "id", sentry_value_new_int32(42));
    sentry_value_set_key(user, "username",
                         sentry_value_new_string("some_name"));
    sentry_set_user(user);

    memset((char *)0x0, 1, 100);
}
