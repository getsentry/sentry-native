#include <sentry.h>
#include <stdlib.h>

int
main(void)
{
    const char *database = getenv("SENTRY_DATABASE_PATH");
    if (!database) {
        return 1;
    }

    sentry_options_t *options = sentry_options_new();
    sentry_options_set_handler_path(options, SENTRY_HANDLER_PATH);
    sentry_options_set_database_path(options, database);
    if (sentry_init(options) != 0) {
        return 1;
    }
    sentry_close();
    return 0;
}
