#include <benchmark/benchmark.h>

extern "C" {
#include "sentry_core.h"
#include "sentry_options.h"
#include "sentry_scope.h"
}

static void
benchmark_scope_set_tag(benchmark::State &state)
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

BENCHMARK(benchmark_scope_set_tag)
    ->Iterations(1000)
    ->Unit(benchmark::kMillisecond);

static void
benchmark_scope_add_breadcrumb(benchmark::State &state)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_init(options);

    int i = 0;
    for (auto _ : state) {
        char msg[32];
        snprintf(msg, sizeof(msg), "message%d", i);
        sentry_add_breadcrumb(sentry_value_new_breadcrumb(NULL, msg));
        i++;
    }

    sentry_close();
}

BENCHMARK(benchmark_scope_add_breadcrumb)
    ->Iterations(1000)
    ->Unit(benchmark::kMillisecond);
