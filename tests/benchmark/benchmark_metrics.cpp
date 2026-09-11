#include <benchmark/benchmark.h>

extern "C" {
#include "sentry.h"
}

static void
discard_envelope(sentry_envelope_t *envelope, void *)
{
    sentry_envelope_free(envelope);
}

static void
setup_metrics(const benchmark::State &)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_release(options, "benchmark@1.0");
    sentry_options_set_environment(options, "test");
    sentry_options_set_auto_session_tracking(options, 0);
    sentry_options_set_transport(
        options, sentry_transport_new(discard_envelope));
    sentry_init(options);

    sentry_set_attribute("global",
        sentry_value_new_attribute(sentry_value_new_string("attribute"), NULL));
}

static void
teardown_metrics(const benchmark::State &)
{
    sentry_close();
}

static void
benchmark_metrics(benchmark::State &state)
{
    sentry_value_t attributes = sentry_value_new_object();
    sentry_value_set_by_key(attributes, "asset.type",
        sentry_value_new_attribute(sentry_value_new_string("texture"), NULL));

    int failed = 0;
    for (auto _ : state) {
        if (sentry_metrics_distribution("asset.load.duration", 12.5,
                SENTRY_UNIT_MILLISECOND, sentry_value_incref(attributes))
            != SENTRY_METRICS_RESULT_SUCCESS) {
            failed++;
        }
    }
    state.SetItemsProcessed(state.iterations() - failed);
    state.counters["enqueue_failures"] = failed;

    sentry_value_decref(attributes);
}

BENCHMARK(benchmark_metrics)
    ->Threads(1)
    ->Threads(8)
    ->Threads(16)
    ->Threads(32)
    ->Iterations(32) // 32x32=1024 (peak) > 10x100=1000 (capacity)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond)
    ->Setup(setup_metrics)
    ->Teardown(teardown_metrics);
