import os
import shutil

import pytest

from . import make_dsn, run


def run_benchmark(target, backend, cmake, httpserver, gbenchmark, label, runs=1):
    tmp_path = cmake(
        ["sentry_benchmark"],
        {
            "SENTRY_BACKEND": backend,
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

        # ignore warmup run
        if runs == 1 or i > 0:
            gbenchmark(benchmark_out, label)


@pytest.mark.parametrize("backend", ["inproc", "breakpad", "crashpad"])
def test_benchmark_init(backend, cmake, httpserver, gbenchmark):
    run_benchmark(
        "init", backend, cmake, httpserver, gbenchmark, f"SDK init ({backend})", 6
    )


@pytest.mark.parametrize("backend", ["inproc", "breakpad", "crashpad"])
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


@pytest.mark.parametrize("test_name", ["apply", "capture"])
def test_benchmark_scope(cmake, httpserver, gbenchmark, test_name):
    run_benchmark(
        f"benchmark_scope_{test_name}",
        "none",
        cmake,
        httpserver,
        gbenchmark,
        f"Scope ({test_name})",
    )


@pytest.mark.parametrize(
    "test_name",
    [
        "log_attributes",
        "flat_object",
        "nested_object",
        "flat_list",
    ],
)
def test_benchmark_value(cmake, httpserver, gbenchmark, test_name):
    run_benchmark(
        f"benchmark_value_{test_name}",
        "none",
        cmake,
        httpserver,
        gbenchmark,
        f"Value ({test_name})",
    )
