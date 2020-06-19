import pytest
import subprocess
import sys
import os
from . import cmake, make_dsn, check_output, run, Envelope
from .conditions import has_http, has_inproc, has_breakpad
from .assertions import (
    assert_attachment,
    assert_meta,
    assert_breadcrumb,
    assert_stacktrace,
    assert_event,
    assert_crash,
    assert_session,
    assert_minidump,
)

if not has_http:
    pytest.skip("tests need http", allow_module_level=True)

auth_header = (
    "Sentry sentry_key=uiaeosnrtdy, sentry_version=7, sentry_client=sentry.native/0.3.3"
)


def test_capture_http(tmp_path, httpserver):
    # we want to have the default transport
    cmake(tmp_path, ["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/", headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    with httpserver.wait(raise_assertions=True, stop_on_nohandler=True) as waiting:
        env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver), SENTRY_RELEASE="🤮🚀")
        run(
            tmp_path,
            "sentry_example",
            ["log", "release-env", "capture-event", "add-stacktrace"],
            check=True,
            env=env,
        )

    assert waiting.result

    output = httpserver.log[0][0].get_data()
    envelope = Envelope.deserialize(output)

    assert_meta(envelope, "🤮🚀")
    assert_breadcrumb(envelope)
    assert_stacktrace(envelope)

    assert_event(envelope)


def test_session_http(tmp_path, httpserver):
    # we want to have the default transport
    cmake(tmp_path, ["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/", headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    # start once without a release, but with a session
    run(
        tmp_path,
        "sentry_example",
        ["log", "release-env", "start-session"],
        check=True,
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )
    run(
        tmp_path,
        "sentry_example",
        ["log", "start-session"],
        check=True,
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) == 1
    output = httpserver.log[0][0].get_data()
    envelope = Envelope.deserialize(output)

    assert_session(envelope, {"init": True, "status": "exited", "errors": 0})


def test_capture_and_session_http(tmp_path, httpserver):
    # we want to have the default transport
    cmake(tmp_path, ["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/", headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "start-session", "capture-event"],
        check=True,
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) == 2
    output = httpserver.log[0][0].get_data()
    envelope = Envelope.deserialize(output)

    assert_event(envelope)
    assert_session(envelope, {"init": True, "status": "ok", "errors": 0})

    output = httpserver.log[1][0].get_data()
    envelope = Envelope.deserialize(output)
    assert_session(envelope, {"status": "exited", "errors": 0})


@pytest.mark.skipif(not has_inproc, reason="test needs inproc backend")
def test_inproc_crash_http(tmp_path, httpserver):
    cmake(tmp_path, ["sentry_example"], {"SENTRY_BACKEND": "inproc"})

    child = run(
        tmp_path, "sentry_example", ["log", "start-session", "attachment", "crash"]
    )
    assert child.returncode  # well, its a crash after all

    httpserver.expect_request(
        "/api/123456/envelope/", headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        check=True,
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) == 2
    outputs = (httpserver.log[0][0].get_data(), httpserver.log[1][0].get_data())
    session, event = (
        outputs if b'"type":"session"' in outputs[0] else (outputs[1], outputs[0])
    )

    envelope = Envelope.deserialize(session)
    assert_session(envelope, {"init": True, "status": "crashed", "errors": 0})

    envelope = Envelope.deserialize(event)
    assert_meta(envelope)
    assert_breadcrumb(envelope)
    assert_attachment(envelope)

    assert_crash(envelope)


@pytest.mark.skipif(not has_inproc, reason="test needs inproc backend")
def test_inproc_dump_inflight(tmp_path, httpserver):
    cmake(tmp_path, ["sentry_example"], {"SENTRY_BACKEND": "inproc"})

    httpserver.expect_request(
        "/api/123456/envelope/", headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    child = run(
        tmp_path, "sentry_example", ["log", "capture-multiple", "crash"], env=env
    )
    assert child.returncode  # well, its a crash after all

    run(tmp_path, "sentry_example", ["log", "no-setup"], check=True, env=env)

    # we trigger 10 normal events, and 1 crash
    assert len(httpserver.log) >= 11


@pytest.mark.skipif(not has_breakpad, reason="test needs breakpad backend")
def test_breakpad_crash_http(tmp_path, httpserver):
    cmake(tmp_path, ["sentry_example"], {"SENTRY_BACKEND": "breakpad"})

    child = run(
        tmp_path, "sentry_example", ["log", "start-session", "attachment", "crash"]
    )
    assert child.returncode  # well, its a crash after all

    httpserver.expect_request(
        "/api/123456/envelope/", headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        check=True,
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) == 2
    outputs = (httpserver.log[0][0].get_data(), httpserver.log[1][0].get_data())
    session, event = (
        outputs if b'"type":"session"' in outputs[0] else (outputs[1], outputs[0])
    )

    envelope = Envelope.deserialize(session)
    assert_session(envelope, {"init": True, "status": "crashed", "errors": 0})

    envelope = Envelope.deserialize(event)

    assert_meta(envelope)
    assert_breadcrumb(envelope)
    assert_attachment(envelope)

    assert_minidump(envelope)


@pytest.mark.skipif(not has_breakpad, reason="test needs breakpad backend")
def test_breakpad_dump_inflight(tmp_path, httpserver):
    cmake(tmp_path, ["sentry_example"], {"SENTRY_BACKEND": "breakpad"})

    httpserver.expect_request(
        "/api/123456/envelope/", headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    child = run(
        tmp_path, "sentry_example", ["log", "capture-multiple", "crash"], env=env
    )
    assert child.returncode  # well, its a crash after all

    run(tmp_path, "sentry_example", ["log", "no-setup"], check=True, env=env)

    # we trigger 10 normal events, and 1 crash
    assert len(httpserver.log) >= 11
