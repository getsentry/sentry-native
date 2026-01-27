import os
import time
import pytest

from . import make_dsn, run
from .conditions import has_breakpad, has_files, has_http

pytestmark = pytest.mark.skipif(
    not has_files or not has_http, reason="tests need local filesystem and http"
)


@pytest.mark.parametrize("cache_keep", [True, False])
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
    ],
)
def test_cache_keep(cmake, httpserver, backend, cache_keep):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": backend})
    cache_dir = tmp_path.joinpath(".sentry-native/cache")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "crash"] + (["cache-keep"] if cache_keep else []),
        expect_failure=True,
        env=env,
    )

    assert not cache_dir.exists() or len(list(cache_dir.glob("*.envelope"))) == 0

    # upload (also caches for inproc/breakpad)
    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"] + (["cache-keep"] if cache_keep else []),
        env=env,
    )

    assert cache_dir.exists() or cache_keep is False
    if cache_keep:
        cache_files = list(cache_dir.glob("*.envelope"))
        assert len(cache_files) == 1


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
    ],
)
def test_cache_max_size(cmake, httpserver, backend):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": backend})
    cache_dir = tmp_path.joinpath(".sentry-native/cache")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    # 5 x 2mb
    for i in range(5):
        run(
            tmp_path,
            "sentry_example",
            ["log", "cache-keep", "crash"],
            expect_failure=True,
            env=env,
        )

        # upload (also caches)
        run(tmp_path, "sentry_example", ["log", "cache-keep", "no-setup"], env=env)

        if cache_dir.exists():
            for f in cache_dir.glob("*.envelope"):
                with open(f, "r+b") as file:
                    file.truncate(2 * 1024 * 1024)

    run(tmp_path, "sentry_example", ["log", "cache-keep", "no-setup"], env=env)

    # max 4mb
    assert cache_dir.exists()
    cache_files = list(cache_dir.glob("*.envelope"))
    assert len(cache_files) <= 2
    assert sum(f.stat().st_size for f in cache_files) <= 4 * 1024 * 1024


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
    ],
)
def test_cache_max_age(cmake, httpserver, backend):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": backend})
    cache_dir = tmp_path.joinpath(".sentry-native/cache")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    # 4 crashes that get fully cached
    for i in range(4):
        run(
            tmp_path,
            "sentry_example",
            ["log", "cache-keep", "crash"],
            expect_failure=True,
            env=env,
        )

        # upload (also caches)
        run(tmp_path, "sentry_example", ["log", "cache-keep", "no-setup"], env=env)

    # 2,4,6,8 days old
    assert cache_dir.exists()
    cache_files = list(cache_dir.glob("*.envelope"))
    assert len(cache_files) == 4
    for i, f in enumerate(cache_files):
        mtime = time.time() - ((i + 1) * 2 * 24 * 60 * 60)
        os.utime(str(f), (mtime, mtime))

    # 5th crash
    run(
        tmp_path,
        "sentry_example",
        ["log", "cache-keep", "crash"],
        expect_failure=True,
        env=env,
    )

    # upload (caches 5th + prunes old files)
    run(tmp_path, "sentry_example", ["log", "cache-keep", "no-setup"], env=env)

    # max 5 days
    cache_files = list(cache_dir.glob("*.envelope"))
    assert len(cache_files) == 3
    for f in cache_files:
        assert time.time() - f.stat().st_mtime <= 5 * 24 * 60 * 60


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
    ],
)
def test_cache_max_items(cmake, backend):
    tmp_path = cmake(
        ["sentry_example"], {"SENTRY_BACKEND": backend, "SENTRY_TRANSPORT": "none"}
    )
    cache_dir = tmp_path.joinpath(".sentry-native/cache")

    for i in range(6):
        run(
            tmp_path,
            "sentry_example",
            ["log", "cache-keep", "crash"],
            expect_failure=True,
        )

    run(
        tmp_path,
        "sentry_example",
        ["log", "cache-keep", "no-setup"],
    )

    # max 5 items
    assert cache_dir.exists()
    cache_files = list(cache_dir.glob("*.envelope"))
    assert len(cache_files) == 5
