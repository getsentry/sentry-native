import pytest
import subprocess
import sys
import os
from . import cmake, make_dsn, check_output, run, event_envelope, Envelope
from .conditions import has_http, has_inproc, has_breakpad
from .assertions import assert_attachment, assert_meta, assert_breadcrumb, assert_stacktrace, assert_event, assert_crash, assert_minidump

if not has_http:
    pytest.skip("tests need http", allow_module_level=True)

def test_capture_http(tmp_path, httpserver):
    # we want to have the default transport
    cmake(tmp_path, ["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/store/", headers={
            "x-sentry-auth": "Sentry sentry_key=uiaeosnrtdy, sentry_version=7, sentry_client=sentry.native/0.2.1"
        }).respond_with_data('OK')

    with httpserver.wait(raise_assertions=True, stop_on_nohandler=True) as waiting:
        run(tmp_path, "sentry_example", ["capture-event", "add-stacktrace"],
            check=True, env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)))

    assert waiting.result

    output = httpserver.log[0][0].get_data()
    envelope = event_envelope(output)

    assert_meta(envelope)
    assert_breadcrumb(envelope)
    assert_stacktrace(envelope)

    assert_event(envelope)

@pytest.mark.skipif(not has_inproc, reason="test needs inproc backend")
def test_inproc_crash_http(tmp_path, httpserver):
    cmake(tmp_path, ["sentry_example"], {"SENTRY_BACKEND": "inproc"})

    child = run(tmp_path, "sentry_example", ["attachment", "crash"])
    assert child.returncode # well, its a crash after all

    httpserver.expect_oneshot_request(
        "/api/123456/store/", headers={
            "x-sentry-auth": "Sentry sentry_key=uiaeosnrtdy, sentry_version=7, sentry_client=sentry.native/0.2.1"
        }).respond_with_data('OK')

    with httpserver.wait(raise_assertions=True, stop_on_nohandler=True) as waiting:
        run(tmp_path, "sentry_example", ["no-setup"],
            check=True, env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)))

    assert waiting.result

    output = httpserver.log[0][0].get_data()
    envelope = Envelope.deserialize(output)

    assert_meta(envelope)
    assert_breadcrumb(envelope)
    assert_attachment(envelope)

    assert_crash(envelope)

@pytest.mark.skipif(not has_breakpad, reason="test needs breakpad backend")
def test_breakpad_crash_http(tmp_path, httpserver):
    cmake(tmp_path, ["sentry_example"], {"SENTRY_BACKEND": "breakpad"})

    child = run(tmp_path, "sentry_example", ["attachment", "crash"])
    assert child.returncode # well, its a crash after all

    httpserver.expect_oneshot_request(
        "/api/123456/store/", headers={
            "x-sentry-auth": "Sentry sentry_key=uiaeosnrtdy, sentry_version=7, sentry_client=sentry.native/0.2.1"
        }).respond_with_data('OK')

    with httpserver.wait(raise_assertions=True, stop_on_nohandler=True) as waiting:
        run(tmp_path, "sentry_example", ["no-setup"],
            check=True, env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)))

    assert waiting.result

    output = httpserver.log[0][0].get_data()
    envelope = Envelope.deserialize(output)

    assert_meta(envelope)
    assert_breadcrumb(envelope)
    assert_attachment(envelope)

    assert_minidump(envelope)
