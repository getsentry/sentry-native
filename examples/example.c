#include "sentry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef SENTRY_PLATFORM_WINDOWS
#    include <Windows.h>
#else
#    include <unistd.h>
#endif

static void
print_envelope(sentry_envelope_t *envelope, void *unused_data)
{
    (void)unused_data;
    size_t size_out = 0;
    char *s = sentry_envelope_serialize(envelope, &size_out);
    printf("%s", s);
    sentry_free(s);
}

static bool
has_arg(int argc, char **argv, const char *arg)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], arg) == 0) {
            return true;
        }
    }
    return false;
}

int
main(int argc, char **argv)
{
    sentry_options_t *options = sentry_options_new();

    sentry_options_set_environment(options, "Production");
    sentry_options_set_release(options, "test-example-release");

    if (has_arg(argc, argv, "log")) {
        sentry_options_set_debug(options, 1);
    }

    if (has_arg(argc, argv, "attachment")) {
        // assuming the example / test is run directly from the cmake build
        // directory
        sentry_options_add_attachment(
            options, "CMakeCache.txt", "./CMakeCache.txt");
    }

    if (has_arg(argc, argv, "stdout")) {
        sentry_options_set_transport(
            options, sentry_new_function_transport(print_envelope, NULL));
    }

    sentry_init(options);

    if (!has_arg(argc, argv, "no-setup")) {
        sentry_set_transaction("test-transaction");
        sentry_set_level(SENTRY_LEVEL_WARNING);
        sentry_set_extra("extra stuff", sentry_value_new_string("some value"));
        sentry_set_extra("…unicode key…",
            // https://xkcd.com/1813/ :-)
            sentry_value_new_string("őá…–🤮🚀¿ 한글 테스트"));
        sentry_set_tag("expected-tag", "some value");
        sentry_set_tag("not-expected-tag", "some value");
        sentry_remove_tag("not-expected-tag");

        sentry_value_t context = sentry_value_new_object();
        sentry_value_set_by_key(
            context, "type", sentry_value_new_string("runtime"));
        sentry_value_set_by_key(
            context, "name", sentry_value_new_string("testing-runtime"));
        sentry_set_context("runtime", context);

        sentry_value_t user = sentry_value_new_object();
        sentry_value_set_by_key(user, "id", sentry_value_new_int32(42));
        sentry_value_set_by_key(
            user, "username", sentry_value_new_string("some_name"));
        sentry_set_user(user);

        sentry_value_t default_crumb
            = sentry_value_new_breadcrumb(0, "default level is info");
        sentry_add_breadcrumb(default_crumb);

        sentry_value_t debug_crumb
            = sentry_value_new_breadcrumb("http", "debug crumb");
        sentry_value_set_by_key(
            debug_crumb, "category", sentry_value_new_string("example!"));
        sentry_value_set_by_key(
            debug_crumb, "level", sentry_value_new_string("debug"));
        sentry_add_breadcrumb(debug_crumb);
    }

    if (has_arg(argc, argv, "overflow-breadcrumbs")) {
        for (size_t i = 0; i < 101; i++) {
            char buffer[4];
            snprintf(buffer, 4, "%zu", i);
            sentry_add_breadcrumb(sentry_value_new_breadcrumb(0, buffer));
        }
    }

    if (has_arg(argc, argv, "capture-multiple")) {
        for (size_t i = 0; i < 10; i++) {
            char buffer[10];
            snprintf(buffer, 10, "Event #%zu", i);

            sentry_value_t event = sentry_value_new_message_event(
                SENTRY_LEVEL_INFO, NULL, buffer);
            sentry_capture_event(event);
        }
    }

    if (has_arg(argc, argv, "sleep")) {
#ifdef SENTRY_PLATFORM_WINDOWS
        Sleep(10 * 1000);
#else
        sleep(10);
#endif
    }

    if (has_arg(argc, argv, "crash")) {
        memset((char *)0x0, 1, 100);
    }

    if (has_arg(argc, argv, "capture-event")) {
        sentry_value_t event = sentry_value_new_message_event(
            SENTRY_LEVEL_INFO, "my-logger", "Hello World!");
        if (has_arg(argc, argv, "add-stacktrace")) {
            sentry_event_value_add_stacktrace(event, NULL, 0);
        }
        sentry_capture_event(event);
    }

    // make sure everything flushes
    sentry_shutdown();
}
