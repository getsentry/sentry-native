import os
import shutil
import subprocess
import sys

import pytest

from . import lib_name, make_dsn, run
from .conditions import has_breakpad, has_crashpad, has_native


def run_benchmark(target, backend, cmake, httpserver, gbenchmark, label, runs=1):
    tmp_path = cmake(
        ["sentry_benchmark"],
        {
            "SENTRY_BACKEND": backend,
            "SENTRY_BATCHER_BUFFER_COUNT": "10",
            "SENTRY_BUILD_BENCHMARKS": "ON",
            "CMAKE_BUILD_TYPE": "Release",
        },
    )

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    benchmark_out = tmp_path / "benchmark.json"

    for i in range(runs):
        # make sure we are isolated from previous runs
        shutil.rmtree(tmp_path / ".sentry-native", ignore_errors=True)

        run(
            tmp_path,
            "sentry_benchmark",
            [f"--benchmark_filter={target}", f"--benchmark_out={benchmark_out}"],
            env=env,
        )

        # ignore warmup run for init/startup benchmarks
        if runs == 1 or i > 0:
            gbenchmark(benchmark_out, label)


@pytest.mark.parametrize("backend", ["inproc", "breakpad", "crashpad", "native"])
def test_benchmark_init(backend, cmake, httpserver, gbenchmark):
    run_benchmark(
        "init", backend, cmake, httpserver, gbenchmark, f"SDK init ({backend})", 6
    )


@pytest.mark.parametrize("backend", ["inproc", "breakpad", "crashpad", "native"])
def test_benchmark_backend(backend, cmake, httpserver, gbenchmark):
    run_benchmark(
        "backend",
        backend,
        cmake,
        httpserver,
        gbenchmark,
        f"Backend startup ({backend})",
        6,
    )


@pytest.mark.parametrize("test_name", ["set_tag", "add_breadcrumb"])
@pytest.mark.parametrize("backend", ["inproc", "breakpad", "crashpad", "native"])
def test_benchmark_scope(test_name, backend, cmake, httpserver, gbenchmark):
    run_benchmark(
        f"benchmark_scope_{test_name}",
        backend,
        cmake,
        httpserver,
        gbenchmark,
        f"Scope {test_name} ({backend})",
    )


@pytest.mark.parametrize("threads", [1, 8, 16, 32])
def test_benchmark_logs(threads, cmake, httpserver, gbenchmark):
    run_benchmark(
        f"^benchmark_logs.*threads:{threads}$",
        "none",
        cmake,
        httpserver,
        gbenchmark,
        f"Logs ({threads} thread{'s' if threads > 1 else ''})",
    )


@pytest.mark.parametrize("backend", ["inproc", "breakpad", "crashpad", "native"])
def test_benchmark_libsize(backend, cmake, gmeasurement):
    tmp_path = cmake(
        ["sentry"],
        {
            "SENTRY_BACKEND": backend,
            "CMAKE_BUILD_TYPE": "Release",
        },
    )

    library = tmp_path / lib_name("sentry")
    size = library.stat().st_size
    assert size > 0
    gmeasurement(
        size,
        f"Library size ({backend})",
        "bytes",
        f"Size {size}b",
        range="linear",
    )


@pytest.mark.parametrize(
    "backend",
    [
        "inproc",
        pytest.param(
            "breakpad",
            marks=pytest.mark.skipif(
                not has_breakpad, reason="breakpad backend not available"
            ),
        ),
        pytest.param(
            "crashpad",
            marks=[
                pytest.mark.skipif(
                    not has_crashpad, reason="crashpad backend not available"
                ),
                pytest.mark.skipif(
                    sys.platform == "darwin",
                    reason="crashpad has no first-chance handler on macOS",
                ),
            ],
        ),
        pytest.param(
            "native",
            marks=pytest.mark.skipif(
                not has_native, reason="native backend not available"
            ),
        ),
    ],
)
def test_benchmark_stack_usage(backend, cmake, gmeasurement):
    tmp_path = cmake(
        ["sentry_stack_usage"],
        {
            "SENTRY_BACKEND": backend,
            "SENTRY_BUILD_BENCHMARKS": "ON",
        },
    )

    result = run(
        tmp_path,
        "sentry_stack_usage",
        [],
        expect_failure=True,
        stdout=subprocess.PIPE,
    )

    prefix = b"[STACK] "
    suffix = b" bytes"
    lines = [line for line in result.stdout.splitlines() if line.strip()]
    assert all(line.startswith(prefix) and line.endswith(suffix) for line in lines)
    measurements = [int(line[len(prefix) : -len(suffix)]) for line in lines]
    assert measurements
    assert all(value >= 0 for value in measurements)

    peak = max(measurements)
    assert peak > 0
    gmeasurement(
        peak,
        f"Stack usage ({backend})",
        "bytes",
        f"Peak {peak}b, Segments {len(measurements)}",
    )
