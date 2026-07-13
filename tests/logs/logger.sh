#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
buffer_counts=("$@")
if (( ${#buffer_counts[@]} == 0 )); then
    buffer_counts=(2 3 4 5 6)
fi

log_counts=(1000 10000 100000)
worker_counts=(0 8 32 128)
sleep_ns_values=(0 1000 1000000)
printf '| %-7s | %-6s | %-7s | %-6s | %-15s |\n' \
    Buffers Logs Threads Sleep Failures
printf '|---------|--------|---------|--------|-----------------|\n'

for buffer_count in "${buffer_counts[@]}"; do
    if [[ ! "$buffer_count" =~ ^[0-9]+$ ]] || (( buffer_count < 2 )); then
        printf 'Invalid buffer count: %s\n' "$buffer_count" >&2
        exit 1
    fi

    build_dir="$root_dir/logger-benchmark-build/$buffer_count"

    cmake -S "$root_dir" -B "$build_dir" \
        -DCMAKE_BUILD_TYPE=Release \
        -DSENTRY_BACKEND=none \
        -DSENTRY_TRANSPORT=none \
        -DSENTRY_BUILD_TESTS=OFF \
        -DSENTRY_BUILD_EXAMPLES=ON \
        -DSENTRY_BATCHER_BUFFERS="$buffer_count" >/dev/null 2>&1
    cmake --build "$build_dir" --config Release --target sentry_logger \
        >/dev/null 2>&1

    logger="$build_dir/sentry_logger"
    if [[ ! -x "$logger" ]]; then
        logger="$build_dir/Release/sentry_logger"
    fi
    if [[ ! -x "$logger" && -x "$logger.exe" ]]; then
        logger="$logger.exe"
    fi
    if [[ ! -x "$logger" ]]; then
        printf 'Could not find sentry_logger in %s\n' "$build_dir" >&2
        exit 1
    fi

    for log_count in "${log_counts[@]}"; do
        for worker_count in "${worker_counts[@]}"; do
            for sleep_index in "${!sleep_ns_values[@]}"; do
                sleep_ns="${sleep_ns_values[$sleep_index]}"
                "$logger" "$log_count" -t "$worker_count" -s "$sleep_ns" \
                    --markdown 2>/dev/null
            done
        done
    done
done
