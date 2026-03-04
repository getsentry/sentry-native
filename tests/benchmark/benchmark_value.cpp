#include <benchmark/benchmark.h>

extern "C" {
#include "sentry_core.h"
}

// Benchmark log emission with attribute cloning.
static void
noop_transport_send(sentry_envelope_t *envelope, void *)
{
    sentry_envelope_free(envelope);
}

static void
benchmark_log_with_attributes(benchmark::State &state)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_enable_logs(options, true);
    sentry_options_set_logs_with_attributes(options, true);
    sentry_transport_t *transport = sentry_transport_new(noop_transport_send);
    sentry_options_set_transport(options, transport);
    sentry_init(options);

    int num_attrs = (int)state.range(0);
    sentry_value_t attrs = sentry_value_new_object();
    for (int i = 0; i < num_attrs; i++) {
        char key[32];
        snprintf(key, sizeof(key), "attr%d", i);
        sentry_value_set_by_key(attrs, key,
            sentry_value_new_attribute(
                sentry_value_new_string("some_value"), NULL));
    }

    for (auto _ : state) {
        sentry_value_incref(attrs);
        log_return_value_t rv = sentry_log_info("test message %d", attrs, 42);
        benchmark::DoNotOptimize(rv);
    }

    sentry_value_decref(attrs);
    sentry_close();
}

BENCHMARK(benchmark_log_with_attributes)
    ->Arg(5)
    ->Arg(20)
    ->Unit(benchmark::kNanosecond);

// Direct clone benchmarks — apples-to-apples comparison of sentry__value_clone.

// Flat object (like tags): {string: string}
static void
benchmark_clone_flat_object(benchmark::State &state)
{
    int n = (int)state.range(0);
    sentry_value_t obj = sentry_value_new_object();
    for (int i = 0; i < n; i++) {
        char key[32], val[32];
        snprintf(key, sizeof(key), "key%d", i);
        snprintf(val, sizeof(val), "value%d", i);
        sentry_value_set_by_key(obj, key, sentry_value_new_string(val));
    }
    for (auto _ : state) {
        sentry_value_t clone = sentry__value_clone(obj);
        benchmark::DoNotOptimize(clone);
        sentry_value_decref(clone);
    }
    sentry_value_decref(obj);
}

BENCHMARK(benchmark_clone_flat_object)
    ->Arg(5)
    ->Arg(20)
    ->Arg(100)
    ->Unit(benchmark::kNanosecond);

// Nested object (like contexts): {string: {string: string}}
static void
benchmark_clone_nested_object(benchmark::State &state)
{
    int n = (int)state.range(0);
    sentry_value_t obj = sentry_value_new_object();
    for (int i = 0; i < n; i++) {
        char key[32];
        snprintf(key, sizeof(key), "ctx%d", i);
        sentry_value_t child = sentry_value_new_object();
        sentry_value_set_by_key(
            child, "name", sentry_value_new_string("some_name"));
        sentry_value_set_by_key(
            child, "version", sentry_value_new_string("1.0"));
        sentry_value_set_by_key(obj, key, child);
    }
    for (auto _ : state) {
        sentry_value_t clone = sentry__value_clone(obj);
        benchmark::DoNotOptimize(clone);
        sentry_value_decref(clone);
    }
    sentry_value_decref(obj);
}

BENCHMARK(benchmark_clone_nested_object)
    ->Arg(3)
    ->Arg(10)
    ->Unit(benchmark::kNanosecond);

// Flat list (like fingerprint): [string, string, ...]
static void
benchmark_clone_flat_list(benchmark::State &state)
{
    int n = (int)state.range(0);
    sentry_value_t list = sentry_value_new_list();
    for (int i = 0; i < n; i++) {
        char val[32];
        snprintf(val, sizeof(val), "fp%d", i);
        sentry_value_append(list, sentry_value_new_string(val));
    }
    for (auto _ : state) {
        sentry_value_t clone = sentry__value_clone(list);
        benchmark::DoNotOptimize(clone);
        sentry_value_decref(clone);
    }
    sentry_value_decref(list);
}

BENCHMARK(benchmark_clone_flat_list)
    ->Arg(3)
    ->Arg(20)
    ->Unit(benchmark::kNanosecond);
