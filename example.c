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
    option.database_path = ".";
    option.debug = 1;

    sentry_init(&option);

    sentry_set_transaction("tran");
    // sentry_set_release("different release than the original");
    sentry_set_level(SENTRY_LEVEL_WARNING);
    sentry_set_extra("extra stuff", "some value");
    sentry_set_tag("expected-tag", "some value");
    sentry_set_tag("not-expected-tag", "some value");
    sentry_remove_tag("not-expected-tag");

    sentry_user_t user;
    sentry_user_clear(&user);
    user.id = "blafasel";
    user.username = "asdfasfsadf";
    sentry_set_user(&user);

    memset((char *)0x0, 1, 100);
}