import os
import time
import pytest

from . import run
from .conditions import has_breakpad, has_files

pytestmark = pytest.mark.skipif(not has_files, reason="tests need local filesystem")


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
    tmp_path = cmake(
        ["sentry_example"], {"SENTRY_BACKEND": backend, "SENTRY_TRANSPORT": "none"}
    )
    cache_dir = tmp_path.joinpath(".sentry-native/cache")

    # capture
    run(
        tmp_path,
        "sentry_example",
        ["log", "crash"] + (["cache-keep"] if cache_keep else []),
        expect_failure=True,
    )

    assert not cache_dir.exists() or len(list(cache_dir.glob("*.envelope"))) == 0

    # cache
    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"] + (["cache-keep"] if cache_keep else []),
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
    tmp_path = cmake(
        ["sentry_example"], {"SENTRY_BACKEND": backend, "SENTRY_TRANSPORT": "none"}
    )
    cache_dir = tmp_path.joinpath(".sentry-native/cache")

    # 5 x 2mb
    for i in range(5):
        run(
            tmp_path,
            "sentry_example",
            ["log", "cache-keep", "crash"],
            expect_failure=True,
        )

        if cache_dir.exists():
            cache_files = list(cache_dir.glob("*.envelope"))
            for f in cache_files:
                with open(f, "r+b") as file:
                    file.truncate(2 * 1024 * 1024)

    run(
        tmp_path,
        "sentry_example",
        ["log", "cache-keep", "no-setup"],
    )

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
def test_cache_max_age(cmake, backend):
    tmp_path = cmake(
        ["sentry_example"], {"SENTRY_BACKEND": backend, "SENTRY_TRANSPORT": "none"}
    )
    cache_dir = tmp_path.joinpath(".sentry-native/cache")

    for i in range(5):
        run(
            tmp_path,
            "sentry_example",
            ["log", "cache-keep", "crash"],
            expect_failure=True,
        )

    # 2,4,6,8,10 days old
    assert cache_dir.exists()
    cache_files = list(cache_dir.glob("*.envelope"))
    for i, f in enumerate(cache_files):
        mtime = time.time() - ((i + 1) * 2 * 24 * 60 * 60)
        os.utime(str(f), (mtime, mtime))

    # 0 days old
    run(
        tmp_path,
        "sentry_example",
        ["log", "cache-keep", "no-setup"],
    )

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
