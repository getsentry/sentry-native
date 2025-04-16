#include "sentry_core.h"
#include "sentry_testsupport.h"

#ifdef SENTRY_PLATFORM_NX
SENTRY_TEST(init_failure)
{
    sentry_options_t *options = sentry_options_new();
#else
static int
transport_startup_fail(
    const sentry_options_t *UNUSED(options), void *UNUSED(state))
{
    return 1;
}

static void
noop_send(const sentry_envelope_t *UNUSED(envelope), void *UNUSED(data))
{
}

SENTRY_TEST(init_failure)
{
    sentry_transport_t *transport
        = sentry_new_function_transport(noop_send, NULL);
    sentry_transport_set_startup_func(transport, transport_startup_fail);

    SENTRY_TEST_OPTIONS_NEW(options);
    sentry_options_set_transport(options, transport);
#endif
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    int rv = sentry_init(options);

    TEST_CHECK(rv != 0);
}
