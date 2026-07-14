#include "sentry.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <climits>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

#ifdef _WIN32
#    include <mmsystem.h>
#    include <windows.h>
#endif

#ifndef SENTRY_BATCHER_BUFFER_COUNT
#    define SENTRY_BATCHER_BUFFER_COUNT 2
#endif
#ifndef SENTRY_BATCHER_BUFFER_SIZE
#    define SENTRY_BATCHER_BUFFER_SIZE 100
#endif

static void
print_usage(const char *program)
{
    std::printf("Usage: %s FRAMES [OPTIONS]\n", program);
    std::printf("  FRAMES                 Positive number of game frames\n");
    std::printf("  -t, --threads N        Worker threads in addition to main "
                "(default: 32)\n");
    std::printf("  -l, --logs-per-thread N\n");
    std::printf("                         Logs from each thread per frame "
                "(default: 2)\n");
    std::printf("  -p, --task-burst N     Extra logs from each thread on burst "
                "frames (default: 0)\n");
    std::printf("  -e, --engine-burst N   Extra engine logs at each transition "
                "(default: 0)\n");
    std::printf("  -b, --burst-every N    Frames between transition starts "
                "(default: 60)\n");
    std::printf("  -d, --burst-frames N   Consecutive task-burst frames per "
                "transition (default: 1)\n");
    std::printf("  -i, --frame-us N       Frame interval in microseconds "
                "(default: 16667)\n");
    std::printf("  --markdown             Print only a Markdown result row\n");
}

static bool
parse_int(const char *value, int min_value, int *out)
{
    if (!value || value[0] == '\0') {
        return false;
    }

    errno = 0;
    char *end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    if (errno != 0 || *end != '\0' || parsed < min_value || parsed > INT_MAX) {
        return false;
    }

    *out = static_cast<int>(parsed);
    return true;
}

static const char *
option_value(const char *arg, const char *short_name, const char *long_name)
{
    const size_t short_len = std::strlen(short_name);
    if (std::strncmp(arg, short_name, short_len) == 0
        && arg[short_len] == '=') {
        return arg + short_len + 1;
    }
    const size_t long_len = std::strlen(long_name);
    if (std::strncmp(arg, long_name, long_len) == 0 && arg[long_len] == '=') {
        return arg + long_len + 1;
    }
    return nullptr;
}

static bool
is_option(const char *arg, const char *short_name, const char *long_name)
{
    return std::strcmp(arg, short_name) == 0
        || std::strcmp(arg, long_name) == 0;
}

#ifdef _WIN32
class timer_resolution_t {
public:
    timer_resolution_t()
        : active_(timeBeginPeriod(1) == TIMERR_NOERROR)
    {
    }

    ~timer_resolution_t()
    {
        if (active_) {
            timeEndPeriod(1);
        }
    }

private:
    bool active_;
};
#endif

int
main(int argc, char **argv)
{
    int frames = 0;
    int worker_threads = 32;
    int logs_per_thread = 2;
    int task_burst = 0;
    int engine_burst = 0;
    int burst_every = 60;
    int burst_frames = 1;
    int frame_us = 16667;
    bool markdown = false;

    if (argc == 1) {
        print_usage(argv[0]);
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (std::strcmp(arg, "--markdown") == 0) {
            markdown = true;
        } else if (is_option(arg, "-t", "--threads")) {
            if (++i >= argc || !parse_int(argv[i], 0, &worker_threads)) {
                print_usage(argv[0]);
                return 1;
            }
        } else if (const char *value = option_value(arg, "-t", "--threads")) {
            if (!parse_int(value, 0, &worker_threads)) {
                print_usage(argv[0]);
                return 1;
            }
        } else if (is_option(arg, "-l", "--logs-per-thread")) {
            if (++i >= argc || !parse_int(argv[i], 1, &logs_per_thread)) {
                print_usage(argv[0]);
                return 1;
            }
        } else if (const char *value
            = option_value(arg, "-l", "--logs-per-thread")) {
            if (!parse_int(value, 1, &logs_per_thread)) {
                print_usage(argv[0]);
                return 1;
            }
        } else if (is_option(arg, "-p", "--task-burst")) {
            if (++i >= argc || !parse_int(argv[i], 0, &task_burst)) {
                print_usage(argv[0]);
                return 1;
            }
        } else if (const char *value
            = option_value(arg, "-p", "--task-burst")) {
            if (!parse_int(value, 0, &task_burst)) {
                print_usage(argv[0]);
                return 1;
            }
        } else if (is_option(arg, "-e", "--engine-burst")) {
            if (++i >= argc || !parse_int(argv[i], 0, &engine_burst)) {
                print_usage(argv[0]);
                return 1;
            }
        } else if (const char *value
            = option_value(arg, "-e", "--engine-burst")) {
            if (!parse_int(value, 0, &engine_burst)) {
                print_usage(argv[0]);
                return 1;
            }
        } else if (is_option(arg, "-b", "--burst-every")) {
            if (++i >= argc || !parse_int(argv[i], 1, &burst_every)) {
                print_usage(argv[0]);
                return 1;
            }
        } else if (const char *value
            = option_value(arg, "-b", "--burst-every")) {
            if (!parse_int(value, 1, &burst_every)) {
                print_usage(argv[0]);
                return 1;
            }
        } else if (is_option(arg, "-d", "--burst-frames")) {
            if (++i >= argc || !parse_int(argv[i], 1, &burst_frames)) {
                print_usage(argv[0]);
                return 1;
            }
        } else if (const char *value
            = option_value(arg, "-d", "--burst-frames")) {
            if (!parse_int(value, 1, &burst_frames)) {
                print_usage(argv[0]);
                return 1;
            }
        } else if (is_option(arg, "-i", "--frame-us")) {
            if (++i >= argc || !parse_int(argv[i], 1, &frame_us)) {
                print_usage(argv[0]);
                return 1;
            }
        } else if (const char *value = option_value(arg, "-i", "--frame-us")) {
            if (!parse_int(value, 1, &frame_us)) {
                print_usage(argv[0]);
                return 1;
            }
        } else if (arg[0] != '-') {
            if (frames != 0 || !parse_int(arg, 1, &frames)) {
                print_usage(argv[0]);
                return 1;
            }
        } else {
            print_usage(argv[0]);
            return 1;
        }
    }

    if (frames == 0) {
        print_usage(argv[0]);
        return 1;
    }

#ifdef _WIN32
    timer_resolution_t timer_resolution;
#endif

    const int total_threads = worker_threads + 1;
    const uint64_t regular_logs = (uint64_t)frames * (uint64_t)total_threads
        * (uint64_t)logs_per_thread;
    const int task_frames_per_cycle = std::min(burst_frames, burst_every);
    const uint64_t full_cycles = (uint64_t)frames / (uint64_t)burst_every;
    const int remaining_frames = frames % burst_every;
    const uint64_t task_burst_frames = task_burst > 0
        ? full_cycles * (uint64_t)task_frames_per_cycle
            + (uint64_t)std::min(remaining_frames, task_frames_per_cycle)
        : 0;
    const uint64_t engine_burst_count = engine_burst > 0
        ? ((uint64_t)frames + (uint64_t)burst_every - 1) / (uint64_t)burst_every
        : 0;
    const uint64_t total_logs = regular_logs
        + task_burst_frames * (uint64_t)total_threads * (uint64_t)task_burst
        + engine_burst_count * (uint64_t)engine_burst;
    const long long capacity = (long long)SENTRY_BATCHER_BUFFER_COUNT
        * (long long)SENTRY_BATCHER_BUFFER_SIZE;

    char thread_label[32];
    if (worker_threads == 0) {
        std::snprintf(thread_label, sizeof(thread_label), "1");
    } else {
        std::snprintf(
            thread_label, sizeof(thread_label), "1+%d", worker_threads);
    }
    char frame_label[32];
    std::snprintf(
        frame_label, sizeof(frame_label), "%.1f ms", frame_us / 1000.0);

    sentry_options_t *options = sentry_options_new();
    sentry_options_set_debug(options, 0);
    if (sentry_init(options) != 0) {
        return 1;
    }
    sentry_flush(5000);

    if (markdown) {
        std::printf("| %-7d | %-6d | %-8lld | %-6d | %-7s | %-6d | %-6d | "
                    "%-6d | %-12d | %-11d | %-9s | %-8llu | ",
            SENTRY_BATCHER_BUFFER_COUNT, SENTRY_BATCHER_BUFFER_SIZE, capacity,
            frames, thread_label, logs_per_thread, task_burst, engine_burst,
            burst_frames, burst_every, frame_label,
            (unsigned long long)total_logs);
    } else {
        std::printf("[LOGGER] sending %llu logs over %d frames at %s: %s "
                    "threads x %d logs/frame, +%d logs/thread for %d of every "
                    "%d frames, %d engine logs at each transition, "
                    "%d buffers x %d items = %lld slots\n",
            (unsigned long long)total_logs, frames, frame_label, thread_label,
            logs_per_thread, task_burst, burst_frames, burst_every,
            engine_burst, SENTRY_BATCHER_BUFFER_COUNT,
            SENTRY_BATCHER_BUFFER_SIZE, capacity);
    }
    std::fflush(stdout);

    struct Result {
        uint64_t failed = 0;
    };
    std::vector<Result> results(total_threads);
    static const char *categories[] = { "LogTemp", "LogNet", "LogStreaming",
        "LogPhysics", "LogAnimation", "LogGame" };

    const auto record_status = [](Result &result, log_return_value_t status) {
        if (status == SENTRY_LOG_RETURN_FAILED) {
            result.failed++;
        }
    };
    const auto send_thread_logs = [&results, &record_status](
                                      int thread_index, int frame, int count) {
        Result &result = results[thread_index];
        const char *category = categories[thread_index
            % (sizeof(categories) / sizeof(*categories))];
        for (int log_index = 0; log_index < count; log_index++) {
            char body[192];
            std::snprintf(body, sizeof(body),
                "[%s] Instrumented task log %d/%d (frame %d, thread "
                "#%d)",
                category, log_index + 1, count, frame + 1, thread_index + 1);

            sentry_value_t attributes = sentry_value_new_object();
            sentry_value_set_by_key(
                attributes, "category", sentry_value_new_string(category));
            record_status(
                result, sentry_log(SENTRY_LEVEL_INFO, body, attributes));
        }
    };
    const auto thread_logs_for_frame = [logs_per_thread, task_burst,
                                           burst_every,
                                           task_frames_per_cycle](int frame) {
        return logs_per_thread
            + (task_burst > 0 && frame % burst_every < task_frames_per_cycle
                    ? task_burst
                    : 0);
    };
    const auto send_engine_burst
        = [&results, &record_status, engine_burst](int frame) {
              Result &result = results[0];
              for (int log_index = 0; log_index < engine_burst; log_index++) {
                  char body[192];
                  std::snprintf(body, sizeof(body),
                      "[LogStreaming] Engine transition log %d/%d (frame %d)",
                      log_index + 1, engine_burst, frame + 1);

                  sentry_value_t attributes = sentry_value_new_object();
                  sentry_value_set_by_key(attributes, "category",
                      sentry_value_new_string("LogStreaming"));
                  record_status(
                      result, sentry_log(SENTRY_LEVEL_INFO, body, attributes));
              }
          };

    std::mutex frame_mutex;
    std::condition_variable frame_condition;
    std::condition_variable frame_done_condition;
    std::condition_variable ready_condition;
    int ready_workers = 0;
    int frame_generation = 0;
    int completed_workers = 0;
    bool stopping = false;
    const std::chrono::microseconds frame_interval(frame_us);

    std::vector<std::thread> workers;
    workers.reserve(worker_threads);
    for (int thread_index = 1; thread_index < total_threads; thread_index++) {
        workers.emplace_back([&, thread_index] {
            std::unique_lock<std::mutex> lock(frame_mutex);
            ready_workers++;
            ready_condition.notify_one();
            int observed_generation = 0;
            while (true) {
                frame_condition.wait(lock, [&] {
                    return stopping || frame_generation != observed_generation;
                });
                if (stopping) {
                    break;
                }

                observed_generation = frame_generation;
                const int frame = observed_generation - 1;
                lock.unlock();
                send_thread_logs(
                    thread_index, frame, thread_logs_for_frame(frame));
                lock.lock();

                completed_workers++;
                if (completed_workers == worker_threads) {
                    frame_done_condition.notify_one();
                }
            }
        });
    }

    {
        std::unique_lock<std::mutex> lock(frame_mutex);
        ready_condition.wait(lock, [&ready_workers, worker_threads] {
            return ready_workers == worker_threads;
        });
    }

    for (int frame = 0; frame < frames; frame++) {
        const auto frame_started = std::chrono::steady_clock::now();
        {
            std::lock_guard<std::mutex> lock(frame_mutex);
            completed_workers = 0;
            frame_generation = frame + 1;
        }
        frame_condition.notify_all();

        send_thread_logs(0, frame, thread_logs_for_frame(frame));
        if (engine_burst > 0 && frame % burst_every == 0) {
            send_engine_burst(frame);
        }

        {
            std::unique_lock<std::mutex> lock(frame_mutex);
            frame_done_condition.wait(
                lock, [&] { return completed_workers == worker_threads; });
        }
        std::this_thread::sleep_until(frame_started + frame_interval);
    }

    {
        std::lock_guard<std::mutex> lock(frame_mutex);
        stopping = true;
    }
    frame_condition.notify_all();

    for (std::thread &worker : workers) {
        worker.join();
    }

    uint64_t failed_count = 0;
    for (const Result &result : results) {
        failed_count += result.failed;
    }
    const double failure_rate
        = total_logs == 0 ? 0.0 : failed_count * 100.0 / total_logs;
    char failure_label[32];
    std::snprintf(failure_label, sizeof(failure_label), "%llu (%.1f%%)",
        (unsigned long long)failed_count, failure_rate);
    if (markdown) {
        std::printf("%-15s |\n", failure_label);
    } else {
        std::printf("[LOGGER] | %-7d | %-6d | %-8lld | %-6d | %-7s | %-6d | "
                    "%-6d | %-6d | %-12d | %-11d | %-9s | %-8llu | %-15s |\n",
            SENTRY_BATCHER_BUFFER_COUNT, SENTRY_BATCHER_BUFFER_SIZE, capacity,
            frames, thread_label, logs_per_thread, task_burst, engine_burst,
            burst_frames, burst_every, frame_label,
            (unsigned long long)total_logs, failure_label);
    }
    std::fflush(stdout);

    sentry_close();
    return 0;
}
