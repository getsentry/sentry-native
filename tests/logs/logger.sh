#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
buffer_configs=("$@")
if (( ${#buffer_configs[@]} == 0 )); then
    buffer_configs=(
        2x100
        2x200
        3x100
        3x200
        4x100
        4x200
    )
fi

# frames workers logs/thread task-burst engine-burst burst-every burst-frames
read -r frames workers logs_per_thread task_burst engine_burst burst_every \
    burst_frames <<<"120 128 2 16 512 30 8"
runs=10

failure_rate() {
    awk -v failures="$1" -v total="$2" \
        'BEGIN { printf "%.1f", failures * 100.0 / total }'
}

printf 'Workload: 120 frames, 129 producers, 2 base logs/producer/frame, +16 for 8/30 transition frames, and 512 engine logs/transition. Median of 10 runs.\n\n'
printf '| %-7s | %-6s | %-8s | %-14s | %-12s |\n' \
    Buffers Size Capacity 'Median' 'Min-Max'
printf '|---------|--------|----------|----------------|--------------|\n'

for buffer_config in "${buffer_configs[@]}"; do
    if [[ "$buffer_config" == *x* ]]; then
        buffer_count="${buffer_config%%x*}"
        buffer_size="${buffer_config#*x}"
    else
        buffer_count="$buffer_config"
        buffer_size=100
    fi

    build_dir="$root_dir/logger-benchmark-build/${buffer_count}x${buffer_size}"

    cmake -S "$root_dir" -B "$build_dir" \
        -DCMAKE_BUILD_TYPE=Release \
        -DSENTRY_BACKEND=none \
        -DSENTRY_TRANSPORT=none \
        -DSENTRY_BUILD_TESTS=OFF \
        -DSENTRY_BUILD_EXAMPLES=ON \
        -DSENTRY_BATCHER_BUFFER_COUNT="$buffer_count" \
        -DSENTRY_BATCHER_BUFFER_SIZE="$buffer_size" \
        -DSENTRY_BATCHER_TIMING=OFF >/dev/null 2>&1
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

    failures=()
    total=0
    for (( run = 0; run < runs; run++ )); do
        result="$("$logger" "$frames" -t "$workers" -l "$logs_per_thread" \
            -p "$task_burst" -e "$engine_burst" -b "$burst_every" -i 16667 \
            -d "$burst_frames" --markdown 2>/dev/null)"
        read -r failure total <<<"$(awk -F'|' '
            {
                gsub(/^[[:space:]]+|[[:space:]]+$/, "", $13)
                gsub(/^[[:space:]]+|[[:space:]]+$/, "", $14)
                split($14, failure, " ")
                print failure[1], $13
            }
        ' <<<"$result")"
        failures+=("$failure")
    done

    sorted_failures=($(printf '%s\n' "${failures[@]}" | sort -n))
    minimum="${sorted_failures[0]}"
    median="${sorted_failures[1]}"
    maximum="${sorted_failures[2]}"
    median_label="$median ($(failure_rate "$median" "$total")%)"
    range_label="$(failure_rate "$minimum" "$total")%-$(failure_rate "$maximum" "$total")%"
    capacity=$(( buffer_count * buffer_size ))

    printf '| %-7d | %-6d | %-8d | %-14s | %-12s |\n' \
        "$buffer_count" "$buffer_size" "$capacity" "$median_label" \
        "$range_label"
done
