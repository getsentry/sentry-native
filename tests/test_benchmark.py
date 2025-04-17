import os
import sys
import shutil

import pytest

from . import make_dsn, run


@pytest.mark.parametrize("backend", ["none", "inproc", "breakpad", "crashpad"])
def test_benchmark(cmake, httpserver, gbenchmark, backend):
    tmp_path = cmake(
        ["sentry_benchmark"],
        {"SENTRY_BACKEND": backend, "SENTRY_BUILD_BENCHMARKS": "ON"},
    )

    # make sure we are isolated from previous runs
    shutil.rmtree(tmp_path / ".sentry-native", ignore_errors=True)

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    run(
        tmp_path,
        "sentry_benchmark",
        [f"--benchmark_out=benchmark.json"],
        check=True,
        env=env,
    )

    gbenchmark(tmp_path / "benchmark.json")
