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

static sentry_options_t *
init_sdk()
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_transport_t *transport = sentry_transport_new(noop_transport_send);
    sentry_options_set_transport(options, transport);
    sentry_init(options);

    sentry_set_user(
        sentry_value_new_user("42", "jdoe", "j@example.com", "127.0.0.1"));
    sentry_set_fingerprint("fp1", "fp2", "fp3", NULL);

    for (int i = 0; i < 50; i++) {
        char key[32], val[32];
        snprintf(key, sizeof(key), "tag%d", i);
        snprintf(val, sizeof(val), "value%d", i);
        sentry_set_tag(key, val);
    }

    sentry_value_t os = sentry_value_new_object();
    sentry_value_set_by_key(os, "name", sentry_value_new_string("macOS"));
    sentry_value_set_by_key(os, "version", sentry_value_new_string("14.0"));
    sentry_set_context("os", os);

    sentry_value_t device = sentry_value_new_object();
    sentry_value_set_by_key(
        device, "model", sentry_value_new_string("MacBookPro"));
    sentry_value_set_by_key(
        device, "family", sentry_value_new_string("Macintosh"));
    sentry_value_set_by_key(device, "arch", sentry_value_new_string("arm64"));
    sentry_set_context("device", device);

    sentry_value_t gpu = sentry_value_new_object();
    sentry_value_set_by_key(
        gpu, "name", sentry_value_new_string("Apple M1 Pro"));
    sentry_value_set_by_key(
        gpu, "vendor_name", sentry_value_new_string("Apple"));
    sentry_set_context("gpu", gpu);

    return options;
}

// Benchmark scope_apply_to_event — the core of every event capture.
// Clones tags, extra, contexts, user, fingerprint from scope to event.
static void
benchmark_scope_apply(benchmark::State &state)
{
    init_sdk();

    for (auto _ : state) {
        sentry_value_t event = sentry_value_new_object();
        const sentry_scope_t *scope = sentry__scope_lock();
        const sentry_options_t *opts = sentry__options_getref();
        sentry__scope_apply_to_event(scope, opts, event, SENTRY_SCOPE_NONE);
        sentry_options_free((sentry_options_t *)opts);
        sentry__scope_unlock();
        benchmark::DoNotOptimize(event);
        sentry_value_decref(event);
    }

    sentry_close();
}

BENCHMARK(benchmark_scope_apply);

// Benchmark full event capture with a noop transport.
// Includes scope_apply, before_send, envelope creation, etc.
static void
benchmark_scope_capture(benchmark::State &state)
{
    init_sdk();

    for (auto _ : state) {
        sentry_value_t event
            = sentry_value_new_message_event(SENTRY_LEVEL_INFO, NULL, "test");
        sentry_uuid_t id = sentry_capture_event(event);
        benchmark::DoNotOptimize(id);
    }

    sentry_close();
}

BENCHMARK(benchmark_scope_capture);
