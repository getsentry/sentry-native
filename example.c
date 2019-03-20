#include <stdio.h>
#include <string.h>
#include "sentry.h"

int main(void) {
    sentry_options_t option;
    sentry_options_init(&option);

    option.dsn = "https://93b6c4c0c1a14bec977f0f1adf8525e6@sentry.garcia.in/3";
    option.handler_path = "../crashpad-Darwin/bin/crashpad_handler";
    option.environment = "Production";
    option.release = "5fd7a6cd";
    option.dist = "12345";
    option.database_path = "sentrypad-data";
    option.debug = 1;

    const char *attachments[3] = {"file1file1.txt", "file2=file2.txt", NULL};
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

    for (int i = 0; i < 50; i++) {
        char buffer[4];
        sprintf(buffer, "%d", i);
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