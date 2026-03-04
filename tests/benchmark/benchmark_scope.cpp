#include <benchmark/benchmark.h>

extern "C" {
#include "sentry_core.h"
#include "sentry_options.h"
#include "sentry_scope.h"
}

static void
noop_transport_send(sentry_envelope_t *envelope, void *)
{
    sentry_envelope_free(envelope);
}

static void
populate_scope(int num_tags, int num_extra, bool with_contexts)
{
    for (int i = 0; i < num_tags; i++) {
        char key[32], val[32];
        snprintf(key, sizeof(key), "tag%d", i);
        snprintf(val, sizeof(val), "value%d", i);
        sentry_set_tag(key, val);
    }

    for (int i = 0; i < num_extra; i++) {
        char key[32];
        snprintf(key, sizeof(key), "extra%d", i);
        sentry_value_t obj = sentry_value_new_object();
        sentry_value_set_by_key(
            obj, "data", sentry_value_new_string("some_data"));
        sentry_value_set_by_key(
            obj, "count", sentry_value_new_int32(i));
        sentry_set_extra(key, obj);
    }

    if (with_contexts) {
        sentry_value_t os = sentry_value_new_object();
        sentry_value_set_by_key(
            os, "name", sentry_value_new_string("macOS"));
        sentry_value_set_by_key(
            os, "version", sentry_value_new_string("14.0"));
        sentry_set_context("os", os);

        sentry_value_t device = sentry_value_new_object();
        sentry_value_set_by_key(
            device, "model", sentry_value_new_string("MacBookPro"));
        sentry_value_set_by_key(
            device, "family", sentry_value_new_string("Macintosh"));
        sentry_value_set_by_key(
            device, "arch", sentry_value_new_string("arm64"));
        sentry_set_context("device", device);

        sentry_value_t gpu = sentry_value_new_object();
        sentry_value_set_by_key(
            gpu, "name", sentry_value_new_string("Apple M1 Pro"));
        sentry_value_set_by_key(
            gpu, "vendor_name", sentry_value_new_string("Apple"));
        sentry_set_context("gpu", gpu);
    }

    sentry_set_user(
        sentry_value_new_user("42", "jdoe", "j@example.com", "127.0.0.1"));
    sentry_set_fingerprint("fp1", "fp2", "fp3", NULL);
}

// Benchmark scope_apply_to_event — the core of every event capture.
// Clones tags, extra, contexts, user, fingerprint from scope to event.
static void
benchmark_scope_apply(benchmark::State &state)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_transport_t *transport = sentry_transport_new(noop_transport_send);
    sentry_options_set_transport(options, transport);
    sentry_init(options);

    int num_tags = (int)state.range(0);
    int num_extra = (int)state.range(1);
    populate_scope(num_tags, num_extra, true);

    for (auto _ : state) {
        sentry_value_t event = sentry_value_new_object();
        const sentry_scope_t *scope = sentry__scope_lock();
        const sentry_options_t *opts = sentry__options_getref();
        sentry__scope_apply_to_event(
            scope, opts, event, SENTRY_SCOPE_NONE);
        sentry_options_free((sentry_options_t *)opts);
        sentry__scope_unlock();
        benchmark::DoNotOptimize(event);
        sentry_value_decref(event);
    }

    sentry_close();
}

BENCHMARK(benchmark_scope_apply)
    ->Args({ 5, 3 })
    ->Args({ 20, 10 })
    ->Args({ 50, 20 })
    ->Unit(benchmark::kNanosecond);

// Benchmark full event capture with a noop transport.
// Includes scope_apply, before_send, envelope creation, etc.
static void
benchmark_capture_event(benchmark::State &state)
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_transport_t *transport = sentry_transport_new(noop_transport_send);
    sentry_options_set_transport(options, transport);
    sentry_init(options);

    int num_tags = (int)state.range(0);
    int num_extra = (int)state.range(1);
    populate_scope(num_tags, num_extra, true);

    for (auto _ : state) {
        sentry_value_t event
            = sentry_value_new_message_event(SENTRY_LEVEL_INFO, NULL, "test");
        sentry_uuid_t id = sentry_capture_event(event);
        benchmark::DoNotOptimize(id);
    }

    sentry_close();
}

BENCHMARK(benchmark_capture_event)
    ->Args({ 5, 3 })
    ->Args({ 20, 10 })
    ->Args({ 50, 20 })
    ->Unit(benchmark::kNanosecond);
