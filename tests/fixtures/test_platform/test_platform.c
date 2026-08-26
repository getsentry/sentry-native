#include "sentry_alloc.h"
#include "sentry_integration.h"
#include "sentry_scope.h"

static void
register_platform(
    void *data, sentry_scope_t *scope, const sentry_options_t *options)
{
    (void)data;
    (void)options;

    sentry_value_t device = sentry_value_new_object();
    sentry_value_set_by_key(device, "name", sentry_value_new_string("Test"));
    sentry_value_set_by_key(
        device, "model", sentry_value_new_string("test-model"));
    sentry_value_set_by_key(
        device, "arch", sentry_value_new_string("test-arch"));
    sentry_scope_set_context(scope, "device", device);
}

sentry_integration_t *
sentry_integration_platform_new(void)
{
    sentry_integration_t *integration = SENTRY_MAKE(sentry_integration_t);
    integration->name = "test";
    integration->register_func = register_platform;
    return integration;
}

int
main(void)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_auto_session_tracking(options, 0);
    sentry_options_set_debug(options, true);
    sentry_init(options);

    sentry_set_tag("my-tag", "my-value");
    sentry_set_user(sentry_value_new_user("123", "my-user", NULL, NULL));

    sentry_value_t event = sentry_value_new_message_event(
        SENTRY_LEVEL_INFO, "my-logger", "Hello World!");
    sentry_capture_event(event);

    sentry_close();
}
