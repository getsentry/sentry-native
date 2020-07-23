import pytest
import os
import sys
import time
from . import make_dsn, run, Envelope
from .conditions import has_crashpad
from .assertions import assert_session

if not has_crashpad:
    pytest.skip("test needs crashpad backend", allow_module_level=True)


# TODO:
# Actually assert that we get a correct event/breadcrumbs payload

# Windows and Linux are currently able to flush all the state on crash
flushes_state = sys.platform != "darwin"


def test_crashpad_capture(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "start-session", "capture-event"],
        check=True,
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) == 2


def test_crashpad_crash(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    httpserver.expect_request("/api/123456/minidump/").respond_with_data("OK")
    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    child = run(
        tmp_path,
        "sentry_example",
        ["log", "start-session", "attachment", "overflow-breadcrumbs", "crash"],
        env=env,
    )
    assert child.returncode  # well, its a crash after all

    run(tmp_path, "sentry_example", ["log", "no-setup"], check=True, env=env)

    time.sleep(2)  # lets wait a bit for crashpad sending in the background

    assert len(httpserver.log) == 2
    outputs = (httpserver.log[0][0].get_data(), httpserver.log[1][0].get_data())
    session, multipart = (
        outputs if b'"type":"session"' in outputs[0] else (outputs[1], outputs[0])
    )

    envelope = Envelope.deserialize(session)
    assert_session(
        envelope,
        {
            "status": "crashed" if flushes_state else "abnormal",
            "errors": 1 if flushes_state else 0,
        },
    )

    # TODO: crashpad actually sends a compressed multipart request,
    # which we don’t parse / assert right now.
    # Ideally, we would pull in a real relay to do all the http tests against.


@pytest.mark.skipif(not flushes_state, reason="test needs state flushing")
def test_crashpad_dump_inflight(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    httpserver.expect_request("/api/123456/minidump/").respond_with_data("OK")
    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    child = run(
        tmp_path, "sentry_example", ["log", "capture-multiple", "crash"], env=env
    )
    assert child.returncode  # well, its a crash after all

    run(tmp_path, "sentry_example", ["log", "no-setup"], check=True, env=env)

    time.sleep(2)  # lets wait a bit for crashpad sending in the background

    # we trigger 10 normal events, and 1 crash
    assert len(httpserver.log) >= 11
