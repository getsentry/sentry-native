#include <benchmark/benchmark.h>

#include "sentry_benchmark.h"

extern "C" {
#include "sentry_core.h"
#include "sentry_options.h"
#include "sentry_scope.h"
}

static void
benchmark_breadcrumbs(benchmark::State &state)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_debug(options, true);
    sentry_init(options);
    if (!benchmark_warmup(state, options)) {
        sentry_close();
        return;
    }

    int i = 0;
    for (auto _ : state) {
        char msg[32];
        snprintf(msg, sizeof(msg), "message%d", i);
        sentry_add_breadcrumb(sentry_value_new_breadcrumb(NULL, msg));
        i++;
    }

    sentry_close();
}

BENCHMARK(benchmark_breadcrumbs)
    ->Iterations(1000)
    ->Unit(benchmark::kMillisecond);
