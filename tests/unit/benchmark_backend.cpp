#include <benchmark/benchmark.h>
#include <cstdio>

extern "C" {
#include "sentry_backend.h"
}

static void
benchmark_backend_startup(benchmark::State &state)
{
    sentry_options_t *options = sentry_options_new();
    sentry_backend_t *backend = sentry__backend_new();

    if (!backend) {
        state.SkipWithMessage("no backend");
    } else if (!backend->startup_func) {
        state.SkipWithMessage("no startup_func");
    } else {
        for (auto s : state) {
            backend->startup_func(backend, options);
        }
    }

    sentry__backend_free(backend);
    sentry_options_free(options);
}

BENCHMARK(benchmark_backend_startup)
    ->Iterations(1)
    ->Unit(benchmark::kMillisecond);
