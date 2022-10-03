#include <sentry.h>
#include <stdbool.h>

int
main(int argc, char **argv)
{
    sentry_options_t *options = sentry_options_new();

    sentry_options_set_auto_session_tracking(options, false);
    sentry_options_set_symbolize_stacktraces(options, true);

    sentry_options_set_environment(options, "development");
    // sentry defaults this to the `SENTRY_RELEASE` env variable
    if (!has_arg(argc, argv, "release-env")) {
        sentry_options_set_release(options, "test-example-release");
    }
}
