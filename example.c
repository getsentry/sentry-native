#include <stdio.h>
#include <string.h>
#include "sentry.h"


int main(void) {
    sentry_options_t *options = sentry_options_new();

    sentry_options_set_dsn(
        options, "https://feea6ffd93d44b9f93eeb7e35ab2ff85@sentry.io/287385");
    sentry_options_set_handler_path(options, "bin/Release/crashpad_handler");
    sentry_options_set_environment(options, "Production");
    sentry_options_set_release(options, "5fd7a6cd");
    sentry_options_set_database_path(options, "sentrypad-db");
    sentry_options_set_debug(options, 1);
    sentry_options_add_attachment(options, "example", "example.c");

    sentry_init(options);

    sentry_set_transaction("tran");
    sentry_set_level(SENTRY_LEVEL_WARNING);
    sentry_set_extra("extra stuff", "some value");
    sentry_set_tag("expected-tag", "some value");
    sentry_set_tag("not-expected-tag", "some value");
    sentry_remove_tag("not-expected-tag");
    sentry_set_fingerprint("foo", "bar", NULL);

    sentry_breadcrumb_t default_crumb = {.message = "default level is info"};
    sentry_add_breadcrumb(&default_crumb);

    sentry_breadcrumb_t debug_crumb = {.message = "debug crumb",
                                       .category = "example!",
                                       .type = "http",
                                       .level = SENTRY_LEVEL_DEBUG};
    sentry_add_breadcrumb(&debug_crumb);

    for (size_t i = 0; i < 101; i++) {
        char buffer[4];
        sprintf(buffer, "%zu", i);
        sentry_breadcrumb_t crumb = {
            .message = buffer,
            .level = i % 2 == 0 ? SENTRY_LEVEL_ERROR : SENTRY_LEVEL_WARNING};
        sentry_add_breadcrumb(&crumb);
    }

    sentry_user_t user = {
        .id = "some_id",
        .username = "some name"
    };
    sentry_set_user(&user);

    memset((char *)0x0, 1, 100);
}
