#include <stdio.h>
#include <string.h>
#include "sentry.h"

int main(void) {
    sentry_options_t option;
    sentry_options_init(&option);

    option.dsn = "http://810ca33ccac847b3a39053f3e4303730@127.0.0.1:8000/3";
    option.handler_path = "bin/Release/crashpad_handler";
    option.environment = "Production";
    option.release = "5fd7a6cd";
    option.dist = "12345";
    option.database_path = "sentrypad-db";
    option.debug = 1;

    const char *attachments[3] = {"example=example.c", NULL};
    option.attachments = attachments;

    sentry_init(&option);

    sentry_set_transaction("tran");
    // sentry_set_release("different release than the original");
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

    sentry_user_t user;
    sentry_user_clear(&user);
    user.id = "some id";
    user.username = "some name";
    sentry_set_user(&user);

    memset((char *)0x0, 1, 100);
}
