#include <benchmark/benchmark.h>

#include <atomic>
#include <chrono>

extern "C" {
#include "sentry.h"
#include "sentry_client_report.h"
#include "sentry_ftrace.h"
}

static std::atomic<int64_t> envelopes;

static void
discard_envelope(sentry_envelope_t *envelope, void *)
{
    envelopes.fetch_add(1, std::memory_order_relaxed);
    SENTRY_FTRACE_BEGIN("telemetry.destroy_envelope");
    sentry_envelope_free(envelope);
    SENTRY_FTRACE_END();
}

static void
setup_telemetry(const benchmark::State &)
{
    SENTRY_FTRACE_BEGIN("telemetry.setup");
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://foo@sentry.invalid/42");
    sentry_options_set_release(options, "benchmark@1.0");
    sentry_options_set_environment(options, "test");
    sentry_options_set_logs_with_attributes(options, true);
    sentry_options_set_auto_session_tracking(options, 0);
    sentry_options_set_transport(
        options, sentry_transport_new(discard_envelope));
    sentry_init(options);

    sentry_set_attribute("global",
        sentry_value_new_attribute(sentry_value_new_string("attribute"), NULL));

    // start the batcher and workers before measuring the burst
    sentry_log_info("warmup", sentry_value_new_null(), NULL);
    sentry_metrics_distribution("asset.load.duration", 12.5,
        SENTRY_UNIT_MILLISECOND, sentry_value_new_null());
    sentry_flush(10000);
    envelopes = 0;
    sentry__client_report_reset();
    SENTRY_FTRACE_END();
}

static void
teardown_telemetry(const benchmark::State &)
{
    SENTRY_FTRACE_BEGIN("telemetry.close");
    sentry_close();
    SENTRY_FTRACE_END();
}

static void
benchmark_telemetry(benchmark::State &state)
{
    sentry_value_t logs = sentry_value_new_object();
    sentry_value_set_by_key(logs, "local",
        sentry_value_new_attribute(sentry_value_new_string("attribute"), NULL));
    sentry_value_t metrics = sentry_value_new_object();
    sentry_value_set_by_key(metrics, "asset.type",
        sentry_value_new_attribute(sentry_value_new_string("texture"), NULL));

    int i = 0;
    int64_t log_failures = 0;
    int64_t metric_failures = 0;
    SENTRY_FTRACE_BEGIN("telemetry.produce.%d", state.thread_index());
    for (auto _ : state) {
        if (sentry_log_info("log %d", sentry_value_incref(logs), i++, NULL)
            != SENTRY_LOG_RETURN_SUCCESS) {
            log_failures++;
        }
        if (sentry_metrics_distribution("asset.load.duration", 12.5,
                SENTRY_UNIT_MILLISECOND, sentry_value_incref(metrics))
            != SENTRY_METRICS_RESULT_SUCCESS) {
            metric_failures++;
        }
    }
    SENTRY_FTRACE_END();
    state.SetItemsProcessed(0);
    state.counters["attempted_logs"] = state.iterations();
    state.counters["attempted_metrics"] = state.iterations();
    state.counters["log_failures"] = log_failures;
    state.counters["metric_failures"] = metric_failures;

    // the benchmark loop's exit barrier waits for every producer
    if (state.thread_index() == 0) {
        SENTRY_FTRACE_BEGIN("telemetry.drain");
        const int64_t attempted = 2 * state.iterations() * state.threads();
        const auto start = std::chrono::steady_clock::now();
        if (sentry_flush(10000) != 0) {
            state.SkipWithError("telemetry flush failed");
        }
        const auto elapsed = std::chrono::steady_clock::now() - start;
        state.counters["drain_ms"]
            = std::chrono::duration<double, std::milli>(elapsed).count();
        state.counters["envelopes"] = envelopes.load();
        sentry_client_report_t report;
        sentry__client_report_save(&report);
        int64_t dropped_logs = 0;
        int64_t dropped_metrics = 0;
        for (int reason = 0; reason < SENTRY_DISCARD_REASON_MAX; reason++) {
            dropped_logs
                += report.counts[reason][SENTRY_DATA_CATEGORY_LOG_ITEM];
            dropped_metrics
                += report.counts[reason][SENTRY_DATA_CATEGORY_TRACE_METRIC];
        }
        state.counters["dropped_logs"] = dropped_logs;
        state.counters["dropped_metrics"] = dropped_metrics;
        state.SetItemsProcessed(attempted - dropped_logs - dropped_metrics);
        SENTRY_FTRACE_COUNTER(envelopes.load(), "telemetry.envelopes");
        SENTRY_FTRACE_COUNTER(dropped_logs, "telemetry.dropped_logs");
        SENTRY_FTRACE_COUNTER(dropped_metrics, "telemetry.dropped_metrics");
        SENTRY_FTRACE_END();
    }

    sentry_value_decref(logs);
    sentry_value_decref(metrics);
}

BENCHMARK(benchmark_telemetry)
    ->Threads(1)
    ->Threads(8)
    ->Threads(16)
    ->Threads(32)
    ->Iterations(32) // per category: 32x32=1024 (peak) > 10x100=1000 (capacity)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond)
    ->Setup(setup_telemetry)
    ->Teardown(teardown_telemetry);
