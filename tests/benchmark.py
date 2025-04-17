import os
import shutil

import pytest

from . import make_dsn, run


@pytest.mark.parametrize(
    "target,backend",
    [
        ("init", "inproc"),
        ("init", "breakpad"),
        ("init", "crashpad"),
        ("backend", "inproc"),
        ("backend", "breakpad"),
        ("backend", "crashpad"),
    ],
)
def test_benchmark(target, backend, cmake, httpserver, gbenchmark):
    tmp_path = cmake(
        ["sentry_benchmark"],
        {
            "SENTRY_BACKEND": backend,
            "SENTRY_BUILD_BENCHMARKS": "ON",
            "CMAKE_BUILD_TYPE": "Release",
        },
    )

    # make sure we are isolated from previous runs
    shutil.rmtree(tmp_path / ".sentry-native", ignore_errors=True)

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    benchmark_out = tmp_path / "benchmark.json"

    for i in range(5):
        run(
            tmp_path,
            "sentry_benchmark",
            [f"--benchmark_filter={target}", f"--benchmark_out={benchmark_out}"],
            check=True,
            env=env,
        )
        gbenchmark(benchmark_out)
