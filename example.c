#include <string.h>
#include "sentry.h"

int main(void)
{
    sentry_options_t option;
    sentry_options_init(&option);
    option.dsn = "https://sentry.garcia.in/api/3/minidump/?sentry_key=93b6c4c0c1a14bec977f0f1adf8525e6";
    option.handler_path = "../crashpad-Darwin/bin/crashpad_handler";
    option.environment = "Production";
    option.release = "5fd7a6cd";
    option.dist = "12345";
    option.database_path = ".";

    sentry_init(&option);

    sentry_set_extra("extra stuff", "some value");
    sentry_set_tag("expected-tag", "some value");
    sentry_set_tag("not-expected-tag", "some value");
    sentry_remove_tag("not-expected-tag");

    memset((char *)0x0, 1, 100);
}