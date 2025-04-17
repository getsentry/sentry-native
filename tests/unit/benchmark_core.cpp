#include <benchmark/benchmark.h>
#include <sentry.h>

static void
benchmark_sentry_init(benchmark::State &state)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_debug(options, true);

    for (auto _ : state) {
        sentry_init(options);
    }
    sentry_close();
}

static void
benchmark_sentry_close(benchmark::State &state)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_debug(options, true);
    sentry_init(options);

    for (auto _ : state) {
        sentry_close();
    }
}

BENCHMARK(benchmark_sentry_init)->Iterations(1)->Unit(benchmark::kMillisecond);
BENCHMARK(benchmark_sentry_close)->Iterations(1)->Unit(benchmark::kMillisecond);
