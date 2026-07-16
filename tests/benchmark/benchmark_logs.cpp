#include <benchmark/benchmark.h>

extern "C" {
#include "sentry.h"
}

static void
discard_envelope(sentry_envelope_t *envelope, void *state)
{
    (void)state;
    sentry_envelope_free(envelope);
}

static void
setup_logs(const benchmark::State &)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_logs_with_attributes(options, true);
    sentry_options_set_auto_session_tracking(options, 0);
    sentry_options_set_transport(
        options, sentry_transport_new(discard_envelope));
    sentry_init(options);

    sentry_set_attribute("global",
        sentry_value_new_attribute(sentry_value_new_string("attribute"), NULL));
}

static void
teardown_logs(const benchmark::State &)
{
    sentry_close();
}

static void
benchmark_logs(benchmark::State &state)
{
    sentry_value_t attributes = sentry_value_new_object();
    sentry_value_set_by_key(attributes, "local",
        sentry_value_new_attribute(sentry_value_new_string("attribute"), NULL));

    int i = 0;
    for (auto _ : state) {
        sentry_log_info("log %d", sentry_value_incref(attributes), i++, NULL);
    }

    sentry_value_decref(attributes);
}

BENCHMARK(benchmark_logs)
    ->Threads(1)
    ->Threads(8)
    ->Threads(16)
    ->Threads(32)
    ->Iterations(100)
    ->Unit(benchmark::kMillisecond)
    ->Setup(setup_logs)
    ->Teardown(teardown_logs);
