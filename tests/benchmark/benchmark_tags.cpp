#include <benchmark/benchmark.h>

extern "C" {
#include "sentry_core.h"
#include "sentry_options.h"
#include "sentry_scope.h"
}

static void
benchmark_tags(benchmark::State &state)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    // flush both __sentry-event and the external crash report
    sentry_options_set_external_crash_reporter_path(options, ".");
    sentry_options_set_debug(options, true);
    sentry_init(options);

    int i = 0;
    for (auto _ : state) {
        char key[32], val[32];
        snprintf(key, sizeof(key), "tag%d", i);
        snprintf(val, sizeof(val), "value%d", i);
        sentry_set_tag(key, val);
        i++;
    }

    sentry_close();
}

BENCHMARK(benchmark_tags)
    ->Iterations(1000)
    ->Unit(benchmark::kMillisecond);
