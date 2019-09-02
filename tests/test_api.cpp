#include <sentry.h>
#include <vendor/catch.hpp>

TEST_CASE("init and shutdown", "[api]") {
    for (size_t i = 0; i < 10; i++) {
        sentry_options_t *options = sentry_options_new();
        sentry_options_set_environment(options, "release");
        sentry_init(options);
        sentry_shutdown();
    }
}