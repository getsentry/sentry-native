import os
import time
import pytest

from . import run
from .conditions import has_breakpad, has_files, has_http

pytestmark = [
    pytest.mark.skipif(not has_files, reason="tests need local filesystem"),
    pytest.mark.skipif(not has_http, reason="tests need http transport"),
]

unreachable_dsn = "http://uiaeosnrtdy@127.0.0.1:19999/123456"


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
def test_cache_keep(cmake, backend, cache_keep):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": backend})
    cache_dir = tmp_path.joinpath(".sentry-native/cache")
    env = dict(os.environ, SENTRY_DSN=unreachable_dsn)

    # capture
    run(
        tmp_path,
        "sentry_example",
        ["log", "flush", "crash"] + (["cache-keep"] if cache_keep else []),
        expect_failure=True,
        env=env,
    )

    assert not cache_dir.exists() or len(list(cache_dir.glob("*.envelope"))) == 0

    # flush + cache
    run(
        tmp_path,
        "sentry_example",
        ["log", "flush", "no-setup"] + (["cache-keep"] if cache_keep else []),
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
def test_cache_max_size(cmake, backend):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": backend})
    cache_dir = tmp_path.joinpath(".sentry-native/cache")
    env = dict(os.environ, SENTRY_DSN=unreachable_dsn)

    for i in range(5):
        run(
            tmp_path,
            "sentry_example",
            ["log", "cache-keep", "flush", "crash"],
            expect_failure=True,
            env=env,
        )

    # flush + cache
    run(
        tmp_path,
        "sentry_example",
        ["log", "cache-keep", "flush", "no-setup"],
        env=env,
    )

    # 5 x 2mb
    assert cache_dir.exists()
    cache_files = list(cache_dir.glob("*.envelope"))
    for f in cache_files:
        with open(f, "r+b") as file:
            file.truncate(2 * 1024 * 1024)

    # max 4mb
    run(
        tmp_path,
        "sentry_example",
        ["log", "cache-keep", "no-setup"],
        env=env,
    )

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
def test_cache_max_age(cmake, backend):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": backend})
    cache_dir = tmp_path.joinpath(".sentry-native/cache")
    env = dict(os.environ, SENTRY_DSN=unreachable_dsn)

    for i in range(5):
        run(
            tmp_path,
            "sentry_example",
            ["log", "cache-keep", "flush", "crash"],
            expect_failure=True,
            env=env,
        )

    # flush + cache
    run(
        tmp_path,
        "sentry_example",
        ["log", "cache-keep", "flush", "no-setup"],
        env=env,
    )

    # 2,4,6,8,10 days old
    assert cache_dir.exists()
    cache_files = list(cache_dir.glob("*.envelope"))
    for i, f in enumerate(cache_files):
        mtime = time.time() - ((i + 1) * 2 * 24 * 60 * 60)
        os.utime(str(f), (mtime, mtime))

    # max 5 days
    run(
        tmp_path,
        "sentry_example",
        ["log", "cache-keep", "no-setup"],
        env=env,
    )

    cache_files = list(cache_dir.glob("*.envelope"))
    assert len(cache_files) == 2
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
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": backend})
    cache_dir = tmp_path.joinpath(".sentry-native/cache")
    env = dict(os.environ, SENTRY_DSN=unreachable_dsn)

    for i in range(6):
        run(
            tmp_path,
            "sentry_example",
            ["log", "cache-keep", "flush", "crash"],
            expect_failure=True,
            env=env,
        )

    # flush + cache
    run(
        tmp_path,
        "sentry_example",
        ["log", "cache-keep", "flush", "no-setup"],
        env=env,
    )

    # max 5 items
    assert cache_dir.exists()
    cache_files = list(cache_dir.glob("*.envelope"))
    assert len(cache_files) == 5
