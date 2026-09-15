#ifndef SENTRY_BENCHMARK_H_INCLUDED
#define SENTRY_BENCHMARK_H_INCLUDED

#include <benchmark/benchmark.h>
#include <chrono>
#include <cstring>
#include <thread>

extern "C" {
#include "sentry_database.h"
#include "sentry_options.h"
#include "sentry_path.h"
}

static bool
benchmark_warmup(benchmark::State &state, const sentry_options_t *options)
{
#if defined(SENTRY_BACKEND_NATIVE) && !defined(SENTRY_PLATFORM_IOS)
    if (!options->run) {
        state.SkipWithError("SDK initialization failed");
        return false;
    }

    // the native ready signal precedes transport startup
    sentry_path_t *path
        = sentry__path_join_str(options->run->run_path, "sentry-daemon.log");
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    bool ready = false;
    while (path && std::chrono::steady_clock::now() < deadline) {
        char *log = sentry__path_read_to_buffer(path, nullptr);
        ready = log && std::strstr(log, "Entering main loop");
        sentry_free(log);
        if (ready) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    sentry__path_free(path);
    if (!ready) {
        state.SkipWithError("Native daemon did not finish startup");
    }
    return ready;
#else
    (void)state;
    (void)options;
    return true;
#endif
}

#endif
