import os
import pytest
from . import run
from .conditions import has_http, is_wine

# These tests drive the real cross-thread RT-signal sampler and unwind a signal
# frame inside the handler. qemu-user does not emulate thread-targeted signal
# delivery/unwinding faithfully, so the sample never produces frames. They run
# natively (incl. native arm64); skip only on qemu.
_REAL_SIGNAL_TESTS = [
    "app_hang_end_to_end",
    "app_hang_with_all_threads",
    "app_hang_all_threads_during_module_loading",
]


def _skip_if_unsupported(unittest):
    if unittest in _REAL_SIGNAL_TESTS and os.environ.get("TEST_QEMU"):
        pytest.skip(
            f"{unittest} requires real signal delivery (unsupported under qemu-user)"
        )


def test_unit(cmake, unittest):
    if unittest in ["basic_transport_thread_name", "cache_keep"]:
        pytest.skip("excluded from unit test-suite")
    _skip_if_unsupported(unittest)
    cwd = cmake(
        ["sentry_test_unit"],
        {"SENTRY_BACKEND": "none", "SENTRY_TRANSPORT": "none"},
    )
    env = dict(os.environ)
    run(cwd, "sentry_test_unit", ["--no-summary", unittest], env=env)


@pytest.mark.skipif(not has_http, reason="tests need http transport")
def test_unit_transport(cmake, unittest):
    if unittest in [
        "custom_logger",
        "logger_enable_disable_functionality",
        "logger_level",
    ]:
        pytest.skip("excluded from transport test-suite")
    _skip_if_unsupported(unittest)

    cwd = cmake(
        ["sentry_test_unit"],
        {"SENTRY_BACKEND": "none"},
    )
    env = dict(os.environ)
    run(cwd, "sentry_test_unit", ["--no-summary", unittest], env=env)


def test_unit_with_test_path(cmake, unittest):
    if unittest in ["basic_transport_thread_name", "cache_keep"]:
        pytest.skip("excluded from unit test-suite")
    _skip_if_unsupported(unittest)
    cwd = cmake(
        ["sentry_test_unit"],
        {"SENTRY_BACKEND": "none", "SENTRY_TRANSPORT": "none"},
        cflags=['-DSENTRY_TEST_PATH_PREFIX=\\"./\\"'],
    )
    env = dict(os.environ)
    run(cwd, "sentry_test_unit", ["--no-summary", unittest], env=env)
