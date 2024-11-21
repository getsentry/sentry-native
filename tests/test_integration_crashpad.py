import os
import shutil
import subprocess
import sys
import time

import pytest

from . import make_dsn, run, Envelope, is_proxy_running
from .assertions import (
    assert_crashpad_upload,
    assert_session,
    assert_gzip_file_header,
)
from .conditions import has_crashpad

pytestmark = pytest.mark.skipif(not has_crashpad, reason="tests need crashpad backend")

# Windows and Linux are currently able to flush all the state on crash
flushes_state = sys.platform != "darwin"


def test_crashpad_capture(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    # make sure we are isolated from previous runs
    shutil.rmtree(tmp_path / ".sentry-native", ignore_errors=True)

    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "start-session", "capture-event"],
        check=True,
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) == 2


@pytest.mark.parametrize(
    "run_args",
    [
        pytest.param(["http-proxy"]),  # HTTP proxy test runs on all platforms
        pytest.param(
            ["socks5-proxy"],
            marks=pytest.mark.skipif(
                sys.platform not in ["darwin", "linux"],
                reason="SOCKS5 proxy tests are only supported on macOS and Linux",
            ),
        ),
    ],
)
@pytest.mark.parametrize("proxy_status", [(["off"]), (["on"])])
def test_crashpad_crash_proxy(cmake, httpserver, run_args, proxy_status):
    if not shutil.which("mitmdump"):
        pytest.skip("mitmdump is not installed")

    proxy_process = None  # store the proxy process to terminate it later

    try:
        if proxy_status == ["on"]:
            # start mitmdump from terminal
            if run_args == ["http-proxy"]:
                proxy_process = subprocess.Popen(["mitmdump"])
                time.sleep(5)  # Give mitmdump some time to start
                if not is_proxy_running("localhost", 8080):
                    pytest.fail("mitmdump (HTTP) did not start correctly")
            elif run_args == ["socks5-proxy"]:
                proxy_process = subprocess.Popen(["mitmdump", "--mode", "socks5"])
                time.sleep(5)  # Give mitmdump some time to start
                if not is_proxy_running("localhost", 1080):
                    pytest.fail("mitmdump (SOCKS5) did not start correctly")

        tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

        # make sure we are isolated from previous runs
        shutil.rmtree(tmp_path / ".sentry-native", ignore_errors=True)

        env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
        httpserver.expect_oneshot_request("/api/123456/minidump/").respond_with_data(
            "OK"
        )

        try:
            with httpserver.wait(timeout=10) as waiting:
                child = run(
                    tmp_path, "sentry_example", ["log", "crash"] + run_args, env=env
                )
                assert child.returncode  # well, it's a crash after all
        except AssertionError:
            # we fail on macOS/Linux if the http proxy is not running
            if run_args == ["http-proxy"] and proxy_status == ["off"]:
                assert len(httpserver.log) == 0
                return
            # we only fail on linux if the socks5 proxy is not running
            elif run_args == ["socks5-proxy"] and proxy_status == ["off"]:
                assert len(httpserver.log) == 0
                return

        assert waiting.result

        # Apple's NSURLSession will send the request even if the socks proxy fails
        # https://forums.developer.apple.com/forums/thread/705504?answerId=712418022#712418022
        # Windows also provides fallback for http proxies
        assert len(httpserver.log) == 1
    finally:
        if proxy_process:
            proxy_process.terminate()
            proxy_process.wait()


def test_crashpad_reinstall(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    # make sure we are isolated from previous runs
    shutil.rmtree(tmp_path / ".sentry-native", ignore_errors=True)

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_oneshot_request("/api/123456/minidump/").respond_with_data("OK")

    with httpserver.wait(timeout=10) as waiting:
        child = run(tmp_path, "sentry_example", ["log", "reinstall", "crash"], env=env)
        assert child.returncode  # well, it's a crash after all

    assert waiting.result

    run(tmp_path, "sentry_example", ["log", "no-setup"], check=True, env=env)

    assert len(httpserver.log) == 1


@pytest.mark.skipif(
    sys.platform != "win32",
    reason="Test covers Windows-specific crashes which can only be covered via the Crashpad WER module",
)
# this test currently can't run on CI because the Windows-image doesn't properly support WER, if you want to run the
# test locally, invoke pytest with the --with_crashpad_wer option which is matched with this marker in the runtest setup
@pytest.mark.with_crashpad_wer
@pytest.mark.parametrize(
    "run_args",
    [
        # discarding via before-send or on-crash has no consequence for fast-fail crashes because they by-pass SEH and
        # thus the crash-handler gets no chance to invoke the FirstChanceHandler which in turn would trigger our hooks.
        (["stack-buffer-overrun"]),
        (["stack-buffer-overrun", "discarding-before-send"]),
        (["stack-buffer-overrun", "discarding-on-crash"]),
        (["fastfail"]),
        (["fastfail", "discarding-before-send"]),
        (["fastfail", "discarding-on-crash"]),
    ],
)
def test_crashpad_wer_crash(cmake, httpserver, run_args):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    # make sure we are isolated from previous runs
    shutil.rmtree(tmp_path / ".sentry-native", ignore_errors=True)

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_oneshot_request("/api/123456/minidump/").respond_with_data("OK")
    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    with httpserver.wait(timeout=10) as waiting:
        child = run(
            tmp_path,
            "sentry_example",
            ["log", "start-session", "attachment", "overflow-breadcrumbs"] + run_args,
            env=env,
        )
        assert child.returncode  # well, it's a crash after all

    assert waiting.result

    # the session crash heuristic on Mac uses timestamps, so make sure we have
    # a small delay here
    time.sleep(1)

    run(tmp_path, "sentry_example", ["log", "no-setup"], check=True, env=env)

    assert len(httpserver.log) == 2
    outputs = (httpserver.log[0][0], httpserver.log[1][0])
    session, multipart = (
        (outputs[0].get_data(), outputs[1])
        if b'"type":"session"' in outputs[0].get_data()
        else (outputs[1].get_data(), outputs[0])
    )

    envelope = Envelope.deserialize(session)

    assert_session(envelope, {"status": "crashed", "errors": 1})
    assert_crashpad_upload(multipart)

    # Windows throttles WER crash reporting frequency, so let's wait a bit
    time.sleep(1)


@pytest.mark.parametrize(
    "run_args,build_args",
    [
        # if we crash, we want a dump
        ([], {"SENTRY_TRANSPORT_COMPRESSION": "Off"}),
        ([], {"SENTRY_TRANSPORT_COMPRESSION": "On"}),
        # if we crash and before-send doesn't discard, we want a dump
        pytest.param(
            ["before-send"],
            {},
            marks=pytest.mark.skipif(
                sys.platform == "darwin",
                reason="crashpad doesn't provide SetFirstChanceExceptionHandler on macOS",
            ),
        ),
        # if on_crash() is non-discarding, a discarding before_send() is overruled, so we get a dump
        pytest.param(
            ["discarding-before-send", "on-crash"],
            {},
            marks=pytest.mark.skipif(
                sys.platform == "darwin",
                reason="crashpad doesn't provide SetFirstChanceExceptionHandler on macOS",
            ),
        ),
    ],
)
def test_crashpad_dumping_crash(cmake, httpserver, run_args, build_args):
    build_args.update({"SENTRY_BACKEND": "crashpad"})
    tmp_path = cmake(["sentry_example"], build_args)

    # make sure we are isolated from previous runs
    shutil.rmtree(tmp_path / ".sentry-native", ignore_errors=True)

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_oneshot_request("/api/123456/minidump/").respond_with_data("OK")
    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    with httpserver.wait(timeout=10) as waiting:
        child = run(
            tmp_path,
            "sentry_example",
            ["log", "start-session", "attachment", "overflow-breadcrumbs", "crash"]
            + run_args,
            env=env,
        )
        assert child.returncode  # well, it's a crash after all

    assert waiting.result

    # the session crash heuristic on Mac uses timestamps, so make sure we have
    # a small delay here
    time.sleep(1)

    run(tmp_path, "sentry_example", ["log", "no-setup"], check=True, env=env)

    assert len(httpserver.log) == 2
    session, multipart = (
        (httpserver.log[0][0], httpserver.log[1][0])
        if is_session_envelope(httpserver.log[0][0].get_data())
        else (httpserver.log[1][0], httpserver.log[0][0])
    )

    if build_args.get("SENTRY_TRANSPORT_COMPRESSION") == "On":
        assert_gzip_file_header(session.get_data())

    envelope = Envelope.deserialize(session.get_data())
    assert_session(envelope, {"status": "crashed", "errors": 1})
    assert_crashpad_upload(multipart)


def test_crashpad_dumping_stack_overflow(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    # make sure we are isolated from previous runs
    shutil.rmtree(tmp_path / ".sentry-native", ignore_errors=True)

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_oneshot_request("/api/123456/minidump/").respond_with_data("OK")
    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    with httpserver.wait(timeout=10) as waiting:
        child = run(
            tmp_path,
            "sentry_example",
            ["log", "start-session", "attachment", "stack-overflow"],
            env=env,
        )
        assert child.returncode  # well, it's a crash after all

    assert waiting.result

    # the session crash heuristic on Mac uses timestamps, so make sure we have
    # a small delay here
    time.sleep(1)

    run(tmp_path, "sentry_example", ["log", "no-setup"], check=True, env=env)

    assert len(httpserver.log) == 2
    session, multipart = (
        (httpserver.log[0][0], httpserver.log[1][0])
        if is_session_envelope(httpserver.log[0][0].get_data())
        else (httpserver.log[1][0], httpserver.log[0][0])
    )

    envelope = Envelope.deserialize(session.get_data())
    assert_session(envelope, {"status": "crashed", "errors": 1})
    assert_crashpad_upload(multipart)


def is_session_envelope(data):
    return b'"type":"session"' in data


@pytest.mark.skipif(
    sys.platform == "darwin",
    reason="crashpad doesn't provide SetFirstChanceExceptionHandler on macOS",
)
@pytest.mark.parametrize(
    "run_args",
    [(["discarding-before-send"]), (["discarding-on-crash"])],
)
def test_crashpad_non_dumping_crash(cmake, httpserver, run_args):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    # make sure we are isolated from previous runs
    shutil.rmtree(tmp_path / ".sentry-native", ignore_errors=True)

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    with httpserver.wait(timeout=5, raise_assertions=False) as waiting:
        child = run(
            tmp_path,
            "sentry_example",
            [
                "log",
                "start-session",
                "attachment",
                "overflow-breadcrumbs",
                "crash",
            ]
            + run_args,
            env=env,
        )
        assert child.returncode  # well, it's a crash after all

    assert waiting.result is False

    # the session crash heuristic on Mac uses timestamps, so make sure we have
    # a small delay here
    time.sleep(1)

    run(tmp_path, "sentry_example", ["log", "no-setup"], check=True, env=env)

    assert len(httpserver.log) == 1
    output = httpserver.log[0][0]
    session = output.get_data()
    envelope = Envelope.deserialize(session)

    assert_session(envelope, {"status": "abnormal", "errors": 0})


@pytest.mark.skipif(
    sys.platform == "linux", reason="linux clears the signal handlers on shutdown"
)
def test_crashpad_crash_after_shutdown(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    # make sure we are isolated from previous runs
    shutil.rmtree(tmp_path / ".sentry-native", ignore_errors=True)

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_oneshot_request("/api/123456/minidump/").respond_with_data("OK")

    with httpserver.wait(timeout=10) as waiting:
        child = run(
            tmp_path,
            "sentry_example",
            ["log", "crash-after-shutdown"],
            env=env,
        )
        assert child.returncode  # well, it's a crash after all

    assert waiting.result

    # the session crash heuristic on Mac uses timestamps, so make sure we have
    # a small delay here
    time.sleep(1)

    run(tmp_path, "sentry_example", ["log", "no-setup"], check=True, env=env)

    assert len(httpserver.log) == 1

    assert_crashpad_upload(httpserver.log[0][0])


@pytest.mark.skipif(not flushes_state, reason="test needs state flushing")
def test_crashpad_dump_inflight(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    # make sure we are isolated from previous runs
    shutil.rmtree(tmp_path / ".sentry-native", ignore_errors=True)

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_oneshot_request("/api/123456/minidump/").respond_with_data("OK")
    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    with httpserver.wait(timeout=10) as waiting:
        child = run(
            tmp_path, "sentry_example", ["log", "capture-multiple", "crash"], env=env
        )
        assert child.returncode  # well, it's a crash after all

    assert waiting.result

    run(tmp_path, "sentry_example", ["log", "no-setup"], check=True, env=env)

    # we trigger 10 normal events, and 1 crash
    assert len(httpserver.log) >= 11


def test_disable_backend(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    # make sure we are isolated from previous runs
    shutil.rmtree(tmp_path / ".sentry-native", ignore_errors=True)

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    with httpserver.wait(timeout=5, raise_assertions=False) as waiting:
        child = run(
            tmp_path, "sentry_example", ["disable-backend", "log", "crash"], env=env
        )
        # we crash so process should return non-zero
        assert child.returncode

    # crashpad is disabled, and we are only crashing, so we expect the wait to timeout
    assert waiting.result is False

    run(tmp_path, "sentry_example", ["log", "no-setup"], check=True, env=env)

    # crashpad is disabled, and we are only crashing, so we expect no requests
    assert len(httpserver.log) == 0
