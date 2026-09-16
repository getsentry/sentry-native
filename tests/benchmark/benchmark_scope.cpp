#include <benchmark/benchmark.h>

#include <cstdio>

extern "C" {
#include "sentry_options.h"
#include "sentry_scope.h"
#include "sentry_tracing.h"
}

enum payload_type {
    EVENT,
    TRANSACTION,
};

static sentry_value_t
new_object(const char *prefix, size_t count)
{
    sentry_value_t object = sentry_value_new_object();
    for (size_t i = 0; i < count; i++) {
        char key[32];
        char value[128];
        snprintf(key, sizeof(key), "%s-%zu", prefix, i);
        snprintf(value, sizeof(value),
            "%s-value-%zu-0123456789abcdefghijklmnopqrstuvwxyz", prefix, i);
        sentry_value_set_by_key(object, key, sentry_value_new_string(value));
    }
    return object;
}

static sentry_scope_t *
new_scope(void)
{
    sentry_scope_t *scope = sentry_scope_new();
    sentry_scope_set_release(scope, "benchmark@1.0.0");
    sentry_scope_set_environment(scope, "benchmark");
    sentry_scope_set_transaction(scope, "benchmark transaction");
    sentry_scope_set_level(scope, SENTRY_LEVEL_ERROR);

    sentry_value_t user = sentry_value_new_user(
        "benchmark-user", "benchmark", "benchmark@example.com", "127.0.0.1");
    sentry_scope_set_user(scope, user);

    sentry_scope_set_tags(scope, new_object("tag", 50));

    for (size_t i = 0; i < 20; i++) {
        char key[32];
        snprintf(key, sizeof(key), "context-%zu", i);
        sentry_scope_set_context(scope, key, new_object("field", 10));
    }

    sentry_value_t fingerprints = sentry_value_new_list();
    for (size_t i = 0; i < 10; i++) {
        sentry_value_append(
            fingerprints, sentry_value_new_string("benchmark-fingerprint"));
    }
    sentry_scope_set_fingerprints(scope, fingerprints);

    for (size_t i = 0; i < 100; i++) {
        sentry_value_t breadcrumb = sentry_value_new_breadcrumb(
            "benchmark", "representative benchmark breadcrumb");
        sentry_value_set_by_key(breadcrumb, "data", new_object("field", 10));
        sentry_scope_add_breadcrumb(scope, breadcrumb);
    }

    return scope;
}

static sentry_value_t
new_event(size_t frame_count)
{
    sentry_value_t event = sentry_value_new_event();
    sentry_value_set_by_key(
        event, "logger", sentry_value_new_string("benchmark.heavy.event"));

    sentry_value_set_by_key(event, "tags", new_object("tag", 50));

    sentry_value_t frames = sentry_value_new_list();
    for (size_t i = 0; i < frame_count; i++) {
        sentry_value_t frame = sentry_value_new_object();
        sentry_value_set_by_key(frame, "filename",
            sentry_value_new_string("src/benchmark/heavy_event.cpp"));
        sentry_value_set_by_key(frame, "function",
            sentry_value_new_string("benchmark_heavy_event"));
        sentry_value_set_by_key(
            frame, "lineno", sentry_value_new_int32(static_cast<int32_t>(i)));
        sentry_value_set_by_key(frame, "in_app", sentry_value_new_bool(true));
        sentry_value_append(frames, frame);
    }

    sentry_value_t stacktrace = sentry_value_new_object();
    sentry_value_set_by_key(stacktrace, "frames", frames);
    sentry_value_t exception = sentry_value_new_object();
    sentry_value_set_by_key(
        exception, "type", sentry_value_new_string("BenchmarkError"));
    sentry_value_set_by_key(exception, "value",
        sentry_value_new_string("representative heavy event payload"));
    sentry_value_set_by_key(exception, "stacktrace", stacktrace);
    sentry_value_t exceptions = sentry_value_new_list();
    sentry_value_append(exceptions, exception);
    sentry_value_t exception_values = sentry_value_new_object();
    sentry_value_set_by_key(exception_values, "values", exceptions);
    sentry_value_set_by_key(event, "exception", exception_values);

    return event;
}

static sentry_value_t
new_transaction(size_t span_count)
{
    sentry_value_t transaction = sentry_value_new_event();
    sentry_value_set_by_key(
        transaction, "type", sentry_value_new_string("transaction"));
    sentry_value_set_by_key(transaction, "trace_id",
        sentry_value_new_string("4c79f60c11214eb38604f4ae0781bfb2"));
    sentry_value_set_by_key(
        transaction, "span_id", sentry_value_new_string("fa90fdead5f74052"));
    sentry_value_set_by_key(transaction, "transaction",
        sentry_value_new_string("benchmark heavy transaction"));
    sentry_value_set_by_key(
        transaction, "op", sentry_value_new_string("benchmark.transaction"));

    sentry_value_set_by_key(transaction, "tags", new_object("tag", 50));
    sentry_value_set_by_key(transaction, "data", new_object("data", 50));

    sentry_value_t measurements = sentry_value_new_object();
    for (size_t i = 0; i < 20; i++) {
        char key[32];
        snprintf(key, sizeof(key), "measurement-%zu", i);
        sentry_value_t measurement = sentry_value_new_object();
        sentry_value_set_by_key(
            measurement, "value", sentry_value_new_double(i + 0.5));
        sentry_value_set_by_key(measurements, key, measurement);
    }
    sentry_value_set_by_key(transaction, "measurements", measurements);

    sentry_value_t spans = sentry_value_new_list();
    for (size_t i = 0; i < span_count; i++) {
        sentry_value_t span = sentry_value_new_object();
        sentry_value_set_by_key(span, "trace_id",
            sentry_value_new_string("4c79f60c11214eb38604f4ae0781bfb2"));
        sentry_value_set_by_key(
            span, "span_id", sentry_value_new_string("fa90fdead5f74052"));
        sentry_value_set_by_key(
            span, "op", sentry_value_new_string("benchmark"));
        sentry_value_set_by_key(span, "description",
            sentry_value_new_string("representative completed child span"));
        sentry_value_set_by_key(span, "status", sentry_value_new_string("ok"));
        sentry_value_set_by_key(span, "data", new_object("data", 10));
        sentry_value_append(spans, span);
    }
    sentry_value_set_by_key(transaction, "spans", spans);

    return transaction;
}

static void
benchmark_scope_apply(benchmark::State &state)
{
    auto type = static_cast<payload_type>(state.range(0));
    size_t item_count = static_cast<size_t>(state.range(1));
    sentry_options_t *options = sentry_options_new();
    sentry_scope_t *scope = new_scope();
    sentry_transaction_t *transaction = NULL;
    if (type == TRANSACTION) {
        transaction = sentry__transaction_new(new_transaction(item_count));
        sentry_scope_set_transaction_object(scope, transaction);
        item_count = 0;
    }

    for (auto _ : state) {
        state.PauseTiming();
        sentry_value_t value = new_event(item_count);
        state.ResumeTiming();

        sentry__scope_apply_to_event(scope, options, value, SENTRY_SCOPE_ALL);

        state.PauseTiming();
        sentry_value_decref(value);
        state.ResumeTiming();
    }

    sentry_scope_set_transaction_object(scope, NULL);
    sentry__transaction_decref(transaction);
    sentry_scope_free(scope);
    sentry_options_free(options);
}

BENCHMARK(benchmark_scope_apply)
    ->Args({ EVENT, 10 })
    ->Args({ EVENT, 256 })
    ->Args({ TRANSACTION, 100 })
    ->Args({ TRANSACTION, 1000 })
    ->ArgNames({ "type", "count" })
    ->Iterations(1000)
    ->Unit(benchmark::kMillisecond);
