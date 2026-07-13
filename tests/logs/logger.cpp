#include "sentry.h"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

#ifndef SENTRY_BATCHER_BUFFERS
#    define SENTRY_BATCHER_BUFFERS 2
#endif

static void
print_usage(const char *program)
{
    std::printf("Usage: %s LOGS [-t N] [-s NS] [--markdown]\n", program);
    std::printf("  LOGS         Positive number of logs to send\n");
    std::printf("  -t N\n");
    std::printf("               Number of worker threads, excluding main "
                "thread (default: 0)\n");
    std::printf("  -s NS\n");
    std::printf("               Nanoseconds to sleep between log sends "
                "(default: 0)\n");
    std::printf("  --markdown   Print only a streaming Markdown result row\n");
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

static bool
parse_uint64(const char *value, uint64_t *out)
{
    if (!value || value[0] == '\0') {
        return false;
    }

    errno = 0;
    char *end = nullptr;
    const unsigned long long parsed = std::strtoull(value, &end, 10);
    if (errno != 0 || *end != '\0'
        || parsed > static_cast<unsigned long long>(LLONG_MAX)) {
        return false;
    }

    *out = static_cast<uint64_t>(parsed);
    return true;
}

static const char *
option_value(const char *arg, const char *name)
{
    const size_t name_len = std::strlen(name);
    if (std::strncmp(arg, name, name_len) == 0 && arg[name_len] == '=') {
        return arg + name_len + 1;
    }
    return nullptr;
}

int
main(int argc, char **argv)
{
    int log_entries = 0;
    int log_threads = 0;
    uint64_t sleep_ns = 0;
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
        } else if (std::strcmp(arg, "-t") == 0) {
            if (++i >= argc || !parse_int(argv[i], 0, &log_threads)) {
                print_usage(argv[0]);
                return 1;
            }
        } else if (const char *value = option_value(arg, "-t")) {
            if (!parse_int(value, 0, &log_threads)) {
                print_usage(argv[0]);
                return 1;
            }
        } else if (std::strcmp(arg, "-s") == 0) {
            if (++i >= argc || !parse_uint64(argv[i], &sleep_ns)) {
                print_usage(argv[0]);
                return 1;
            }
        } else if (const char *value = option_value(arg, "-s")) {
            if (!parse_uint64(value, &sleep_ns)) {
                print_usage(argv[0]);
                return 1;
            }
        } else if (arg[0] != '-') {
            if (log_entries != 0) {
                print_usage(argv[0]);
                return 1;
            }
            if (!parse_int(arg, 1, &log_entries)) {
                print_usage(argv[0]);
                return 1;
            }
        } else {
            print_usage(argv[0]);
            return 1;
        }
    }

    if (log_entries == 0) {
        print_usage(argv[0]);
        return 1;
    }

    const int total_threads = log_threads + 1;
    char thread_label[32];
    if (log_threads == 0) {
        std::snprintf(thread_label, sizeof(thread_label), "1");
    } else {
        std::snprintf(thread_label, sizeof(thread_label), "1+%d", log_threads);
    }
    char sleep_label[32];
    if (sleep_ns == 0) {
        std::snprintf(sleep_label, sizeof(sleep_label), "-");
    } else if (sleep_ns % 1000000 == 0) {
        std::snprintf(sleep_label, sizeof(sleep_label), "%llu ms",
            static_cast<unsigned long long>(sleep_ns / 1000000));
    } else if (sleep_ns % 1000 == 0) {
        std::snprintf(sleep_label, sizeof(sleep_label), "%llu us",
            static_cast<unsigned long long>(sleep_ns / 1000));
    } else {
        std::snprintf(sleep_label, sizeof(sleep_label), "%llu ns",
            static_cast<unsigned long long>(sleep_ns));
    }

    sentry_options_t *options = sentry_options_new();
    sentry_options_set_debug(options, 0);
    if (sentry_init(options) != 0) {
        return 1;
    }
    sentry_flush(5000);

    if (markdown) {
        std::printf("| %-7d | %-6d | %-7s | %-6s | ", SENTRY_BATCHER_BUFFERS,
            log_entries, thread_label, sleep_label);
    } else {
        std::printf("[LOGGER] sending %d logs across %d threads (%d workers)\n",
            log_entries, total_threads, log_threads);
    }
    std::fflush(stdout);

    std::atomic<int> success { 0 };
    std::atomic<int> failed { 0 };
    const auto sleep_interval = std::chrono::nanoseconds(sleep_ns);

    const auto send_logs = [&success, &failed, log_entries, sleep_interval](
                               int thread_index, int count, int offset) {
        for (int i = 0; i < count; i++) {
            char body[128];
            if (thread_index == 0) {
                std::snprintf(body, sizeof(body),
                    "sentry-native stress log %d/%d", offset + i + 1,
                    log_entries);
            } else {
                std::snprintf(body, sizeof(body),
                    "sentry-native stress log %d/%d (thread #%d)",
                    offset + i + 1, log_entries, thread_index + 1);
            }

            const log_return_value_t result
                = sentry_log(SENTRY_LEVEL_INFO, body, sentry_value_new_null());
            if (result == SENTRY_LOG_RETURN_SUCCESS) {
                success++;
            } else if (result == SENTRY_LOG_RETURN_FAILED) {
                failed++;
            }
            if (sleep_interval.count() > 0 && i + 1 < count) {
                std::this_thread::sleep_for(sleep_interval);
            }
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(log_threads);
    const int entries_per_thread = log_entries / total_threads;
    const int extra_entries = log_entries % total_threads;
    const int runner_entries = entries_per_thread + (extra_entries > 0 ? 1 : 0);
    int offset = runner_entries;

    for (int thread_index = 0; thread_index < log_threads; thread_index++) {
        const int stress_thread_index = thread_index + 1;
        const int thread_entries = entries_per_thread
            + (stress_thread_index < extra_entries ? 1 : 0);
        const int thread_offset = offset;
        offset += thread_entries;
        workers.emplace_back(
            send_logs, stress_thread_index, thread_entries, thread_offset);
    }

    send_logs(0, runner_entries, 0);
    for (std::thread &worker : workers) {
        worker.join();
    }

    const int success_count = success.load();
    const int failed_count = failed.load();
    const int total = success_count + failed_count;
    const double failure_rate = total == 0 ? 0.0 : failed_count * 100.0 / total;
    char failure_label[32];
    std::snprintf(failure_label, sizeof(failure_label), "%d (%.1f%%)",
        failed_count, failure_rate);
    if (markdown) {
        std::printf("%-15s |\n", failure_label);
    } else {
        std::printf("[LOGGER] | %-7d | %-6d | %-7s | %-6s | %-15s |\n",
            SENTRY_BATCHER_BUFFERS, log_entries, thread_label, sleep_label,
            failure_label);
    }
    std::fflush(stdout);

    sentry_close();
    return 0;
}
