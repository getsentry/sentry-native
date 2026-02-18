#!/usr/bin/env bash
#
# Stress-tests a unit test by running it repeatedly on a single CPU core
# under heavy system load to expose timing-sensitive flakes.
#
# Requires: stress, taskset, nice (Linux only)
#
# Usage: scripts/stress-test.sh [-n iterations] [-s] [test_name...]
# Example: scripts/stress-test.sh -n 10 basic_logging_functionality
# Example: scripts/stress-test.sh -s slow_test  (run all except slow_test)

ITERATIONS=1
SKIP_MODE=0
while getopts "n:s" opt; do
    case $opt in
        n) ITERATIONS="$OPTARG" ;;
        s) SKIP_MODE=1 ;;
        *) echo "Usage: $0 [-n iterations] [-s] [test_name...]" >&2; exit 1 ;;
    esac
done
shift $((OPTIND - 1))
TEST_NAMES=("$@")
TEST_BIN="unit-build/sentry_test_unit"

if [ ! -x "$TEST_BIN" ]; then
    echo "Error: $TEST_BIN not found. Build unit tests first." >&2
    exit 1
fi

for cmd in stress taskset; do
    if ! command -v "$cmd" &>/dev/null; then
        echo "Error: $cmd is required but not found." >&2
        exit 1
    fi
done

if [ ${#TEST_NAMES[@]} -eq 0 ]; then
    echo "Stress-testing all tests ($ITERATIONS iterations)..."
elif [ "$SKIP_MODE" -eq 1 ]; then
    echo "Stress-testing all except: ${TEST_NAMES[*]} ($ITERATIONS iterations)..."
else
    echo "Stress-testing ${TEST_NAMES[*]} ($ITERATIONS iterations)..."
fi

stress --cpu 8 --io 8 --vm 2 --vm-bytes 256M &>/dev/null &
STRESS_PID=$!
trap 'kill $STRESS_PID 2>/dev/null; wait $STRESS_PID 2>/dev/null' EXIT

sleep 0.5
for pid in $(pgrep -P "$STRESS_PID"); do
    taskset -pc 0 "$pid" >/dev/null
done

run_test() {
    taskset -c 0 nice -n 19 "./$TEST_BIN" "$@" || FAIL=$((FAIL + 1))
}

FAIL=0
for i in $(seq 1 "$ITERATIONS"); do
    if [ "$SKIP_MODE" -eq 1 ]; then
        run_test --skip "${TEST_NAMES[@]}"
    elif [ ${#TEST_NAMES[@]} -gt 0 ]; then
        for test in "${TEST_NAMES[@]}"; do
            run_test "$test"
        done
    else
        run_test
    fi
done

echo "=== $FAIL failures out of $ITERATIONS runs ==="
[ "$FAIL" -eq 0 ]
