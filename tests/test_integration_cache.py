import os
import time
import pytest

from . import make_dsn, run, run_crash
from .assertions import wait_for, wait_for_file
from .conditions import has_breakpad, has_files, has_http, has_native, is_qemu

pytestmark = [
    pytest.mark.skipif(not has_files, reason="tests need local filesystem"),
    pytest.mark.skipif(not has_http, reason="tests need http transport"),
]


@pytest.mark.parametrize(
    "cache_args,expect_cache",
    [
        pytest.param([], False, id="none"),
        pytest.param(["cache-keep"], True, id="offline"),
        pytest.param(["cache-keep-always"], True, id="always"),
    ],
)
@pytest.mark.parametrize(
    "backend",
    [
        "inproc",
        pytest.param(
            "breakpad",
            marks=pytest.mark.skipif(
                not has_breakpad or is_qemu, reason="breakpad backend not available"
            ),
        ),
    ],
)
def test_cache_keep(cmake, backend, cache_args, expect_cache, unreachable_dsn):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": backend})
    cache_dir = tmp_path.joinpath(".sentry-native/cache")
    env = dict(os.environ, SENTRY_DSN=unreachable_dsn)

    # capture
    run(
        tmp_path,
        "sentry_example",
        ["log", "no-http-retry", "flush", "crash"] + cache_args,
        expect_failure=True,
        env=env,
    )

    assert not cache_dir.exists() or len(list(cache_dir.glob("*.envelope"))) == 0

    # flush + cache
    run(
        tmp_path,
        "sentry_example",
        ["log", "no-http-retry", "flush", "no-setup"] + cache_args,
        env=env,
    )

    assert cache_dir.exists() or expect_cache is False
    if expect_cache:
        cache_files = list(cache_dir.glob("*.envelope"))
        assert len(cache_files) == 1
        if backend != "inproc":
            dmp_files = list(cache_dir.glob("*.dmp"))
            assert len(dmp_files) == 1
            assert cache_files[0].stem == dmp_files[0].stem


@pytest.mark.skipif(
    not has_native or is_qemu,
    reason="native backend not available",
)
@pytest.mark.parametrize(
    "cache_args,expect_cache",
    [
        pytest.param([], False, id="none"),
        pytest.param(["cache-keep"], True, id="offline"),
        pytest.param(["cache-keep-always"], True, id="always"),
    ],
)
def test_cache_keep_native(cmake, cache_args, expect_cache, unreachable_dsn):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "native"})
    cache_dir = tmp_path / ".sentry-native" / "cache"
    env = dict(os.environ, SENTRY_DSN=unreachable_dsn)

    run_crash(
        tmp_path,
        "sentry_example",
        ["log", "stdout", "crash"] + cache_args,
        env=env,
        wait_for_daemon=not expect_cache,
    )

    if expect_cache:
        assert wait_for_file(cache_dir / "*.envelope")
        cache_files = list(cache_dir.glob("*.envelope"))
        assert len(cache_files) == 1
        dmp_files = list(cache_dir.glob("*.dmp"))
        assert len(dmp_files) == 1
        assert cache_files[0].stem == dmp_files[0].stem
    else:
        assert len(list(cache_dir.glob("*.envelope"))) == 0


def test_cache_keep_always(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "inproc"})
    cache_dir = tmp_path.joinpath(".sentry-native/cache")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    httpserver.expect_oneshot_request("/api/123456/envelope/").respond_with_data("OK")
    with httpserver.wait(timeout=10) as waiting:
        run(
            tmp_path,
            "sentry_example",
            ["log", "cache-keep-always", "flush", "capture-event"],
            env=env,
        )
    assert waiting.result

    cache_files = list(cache_dir.glob("*.envelope"))
    assert len(cache_files) == 1
    assert len(cache_files[0].stem) == 36


@pytest.mark.parametrize(
    "backend",
    [
        "inproc",
        pytest.param(
            "breakpad",
            marks=pytest.mark.skipif(
                not has_breakpad or is_qemu, reason="breakpad backend not available"
            ),
        ),
        pytest.param(
            "native",
            marks=pytest.mark.skipif(
                not has_native or is_qemu, reason="native backend not available"
            ),
        ),
    ],
)
def test_cache_max_size(cmake, backend, unreachable_dsn):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": backend})
    cache_dir = tmp_path.joinpath(".sentry-native/cache")
    env = dict(os.environ, SENTRY_DSN=unreachable_dsn)

    for i in range(5):
        run_crash(
            tmp_path,
            "sentry_example",
            ["log", "no-http-retry", "cache-keep", "flush", "crash"],
            env=env,
            wait_for_daemon=backend == "native",
        )

    # flush + cache
    run(
        tmp_path,
        "sentry_example",
        ["log", "no-http-retry", "cache-keep", "flush", "no-setup"],
        env=env,
    )

    # 5 x 4mb
    assert cache_dir.exists()
    assert wait_for(lambda: len(list(cache_dir.glob("*.envelope"))) == 5)
    cache_files = list(cache_dir.glob("*.envelope"))
    for f in cache_files:
        with open(f, "r+b") as file:
            file.truncate(4 * 1024 * 1024)

    # max 16mb
    run(
        tmp_path,
        "sentry_example",
        ["log", "no-http-retry", "cache-keep", "no-setup"],
        env=env,
    )

    cache_files = list(cache_dir.glob("*.envelope"))
    assert len(cache_files) <= 4
    assert sum(f.stat().st_size for f in cache_files) <= 16 * 1024 * 1024


@pytest.mark.parametrize(
    "backend",
    [
        "inproc",
        pytest.param(
            "breakpad",
            marks=pytest.mark.skipif(
                not has_breakpad or is_qemu, reason="breakpad backend not available"
            ),
        ),
        pytest.param(
            "native",
            marks=pytest.mark.skipif(
                not has_native or is_qemu, reason="native backend not available"
            ),
        ),
    ],
)
def test_cache_max_age(cmake, backend, unreachable_dsn):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": backend})
    cache_dir = tmp_path.joinpath(".sentry-native/cache")
    env = dict(os.environ, SENTRY_DSN=unreachable_dsn)

    for i in range(5):
        run_crash(
            tmp_path,
            "sentry_example",
            ["log", "no-http-retry", "cache-keep", "flush", "crash"],
            env=env,
            wait_for_daemon=backend == "native",
        )

    # flush + cache
    run(
        tmp_path,
        "sentry_example",
        ["log", "no-http-retry", "cache-keep", "flush", "no-setup"],
        env=env,
    )

    # 2,4,6,8,10 days old
    assert cache_dir.exists()
    assert wait_for(lambda: len(list(cache_dir.glob("*.envelope"))) == 5)
    cache_files = list(cache_dir.glob("*.envelope"))
    for i, f in enumerate(cache_files):
        mtime = time.time() - ((i + 1) * 2 * 24 * 60 * 60)
        os.utime(str(f), (mtime, mtime))

    # max 5 days
    run(
        tmp_path,
        "sentry_example",
        ["log", "no-http-retry", "cache-keep", "no-setup"],
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
                not has_breakpad or is_qemu, reason="breakpad backend not available"
            ),
        ),
        pytest.param(
            "native",
            marks=pytest.mark.skipif(
                not has_native or is_qemu, reason="native backend not available"
            ),
        ),
    ],
)
def test_cache_max_items(cmake, backend, unreachable_dsn):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": backend})
    cache_dir = tmp_path.joinpath(".sentry-native/cache")
    env = dict(os.environ, SENTRY_DSN=unreachable_dsn)

    for i in range(6):
        run_crash(
            tmp_path,
            "sentry_example",
            ["log", "no-http-retry", "cache-keep", "flush", "crash"],
            env=env,
            wait_for_daemon=backend == "native",
        )

    # flush + cache
    run(
        tmp_path,
        "sentry_example",
        ["log", "no-http-retry", "cache-keep", "flush", "no-setup"],
        env=env,
    )

    # max 5 items
    assert cache_dir.exists()
    assert wait_for(lambda: len(list(cache_dir.glob("*.envelope"))) == 5)
    cache_files = list(cache_dir.glob("*.envelope"))
    assert len(cache_files) == 5


@pytest.mark.parametrize(
    "backend",
    [
        "inproc",
        pytest.param(
            "breakpad",
            marks=pytest.mark.skipif(
                not has_breakpad or is_qemu, reason="breakpad backend not available"
            ),
        ),
        pytest.param(
            "native",
            marks=pytest.mark.skipif(
                not has_native or is_qemu, reason="native backend not available"
            ),
        ),
    ],
)
def test_cache_max_items_with_retry(cmake, backend, unreachable_dsn):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": backend})
    cache_dir = tmp_path.joinpath(".sentry-native/cache")
    env = dict(os.environ, SENTRY_DSN=unreachable_dsn)

    # Create cache files via crash+restart cycles
    for i in range(4):
        run_crash(
            tmp_path,
            "sentry_example",
            ["log", "cache-keep", "flush", "crash"],
            env=env,
            wait_for_daemon=backend == "native",
        )

    # flush + cache
    run(
        tmp_path,
        "sentry_example",
        ["log", "cache-keep", "flush", "no-setup"],
        env=env,
    )
    assert wait_for(lambda: len(list(cache_dir.glob("*.envelope"))) == 4)

    # Pre-populate cache/ with retry-format envelope files
    cache_dir.mkdir(parents=True, exist_ok=True)
    for i in range(4):
        ts = int(time.time() * 1000)
        f = cache_dir / f"{ts}-00-00000000-0000-0000-0000-{i:012x}.envelope"
        f.write_text("dummy envelope content")

    # Trigger sentry_init which runs cleanup
    run(
        tmp_path,
        "sentry_example",
        ["log", "cache-keep", "no-setup"],
        env=env,
    )

    # max 5 items total in cache/
    assert cache_dir.exists()
    cache_files = list(cache_dir.glob("*.envelope"))
    assert len(cache_files) <= 5


def test_cache_consent_revoke(cmake, unreachable_dsn):
    """With consent revoked and cache_keep, envelopes are cached to disk."""
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})
    cache_dir = tmp_path.joinpath(".sentry-native/cache")
    env = dict(os.environ, SENTRY_DSN=unreachable_dsn)

    run(
        tmp_path,
        "sentry_example",
        [
            "log",
            "cache-keep",
            "require-user-consent",
            "user-consent-revoke",
            "capture-event",
            "flush",
        ],
        env=env,
    )

    assert cache_dir.exists()
    cache_files = list(cache_dir.glob("*.envelope"))
    assert len(cache_files) == 1


def test_cache_consent_discard(cmake, unreachable_dsn):
    """With consent revoked but no cache_keep, envelopes are discarded."""
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})
    cache_dir = tmp_path.joinpath(".sentry-native/cache")
    env = dict(os.environ, SENTRY_DSN=unreachable_dsn)

    run(
        tmp_path,
        "sentry_example",
        [
            "log",
            "require-user-consent",
            "user-consent-revoke",
            "capture-event",
            "flush",
        ],
        env=env,
    )

    assert not cache_dir.exists() or len(list(cache_dir.glob("*.envelope"))) == 0


def test_cache_consent_flush(cmake, httpserver):
    """Giving consent after capturing flushes cached envelopes immediately."""
    from . import make_dsn

    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})
    cache_dir = tmp_path.joinpath(".sentry-native/cache")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        [
            "log",
            "http-retry",
            "require-user-consent",
            "user-consent-revoke",
            "capture-event",
            "user-consent-give",
        ],
        env=env,
    )

    assert len(httpserver.log) >= 1
    assert not cache_dir.exists() or len(list(cache_dir.glob("*.envelope"))) == 0


@pytest.mark.skipif(
    not has_native or is_qemu,
    reason="native backend not available",
)
def test_cache_consent_native(cmake, httpserver):
    """Daemon honors revoked consent for crash envelopes."""
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "native"})
    cache_dir = tmp_path / ".sentry-native" / "cache"
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run_crash(
        tmp_path,
        "sentry_example",
        [
            "log",
            "stdout",
            "cache-keep",
            "http-retry",
            "require-user-consent",
            "user-consent-revoke",
            "crash",
        ],
        env=env,
    )

    assert wait_for_file(cache_dir / "*.envelope")
    assert len(list(cache_dir.glob("*.envelope"))) == 1
    assert len(httpserver.log) == 0

    httpserver.expect_oneshot_request("/api/123456/envelope/").respond_with_data("OK")
    with httpserver.wait(timeout=10) as waiting:
        run(
            tmp_path,
            "sentry_example",
            [
                "log",
                "cache-keep",
                "http-retry",
                "require-user-consent",
                "user-consent-give",
            ],
            env=env,
        )
    assert waiting.result
    assert len(list(cache_dir.glob("*.envelope"))) == 0
