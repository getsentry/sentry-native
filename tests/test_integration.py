import pytest
import subprocess
import sys
import os
from . import cmake, make_dsn, check_output, run, event_envelope, Envelope

# TODO:
# * with inproc backend:
#   - breadcrumbs, attachments, etc
#   - crash
#   - restart, submit via http
#   - restart, submit via stdout
#
# * with crashpad backend:
#   - breadcrumbs, attachments, etc
#   - crash
#   - expect report via http

def matches(actual, expected):
    return {k:v for (k,v) in actual.items() if k in expected.keys()} == expected

def assert_meta(envelope):
    event = envelope.get_event()

    expected = {
        "platform": "native",
        "environment": "Production",
        "contexts": { "runtime": { "type": "runtime", "name": "testing-runtime" } },
        "release": "test-example-release",
        "user": { "id": 42, "username": "some_name" },
        "transaction": "test-transaction",
        "tags": { "expected-tag": "some value" },
        "extra": { "extra stuff": "some value", "…unicode key…": "őá…–🤮🚀¿ 한글 테스트" },
        "sdk": {
            "name": "sentry.native",
            "version": "0.2.1",
            "packages": [
                {
                    "name": "github:getsentry/sentry-native",
                    "version": "0.2.1",
                },
            ],
        },
    }

    assert matches(event, expected)
    assert any("sentry_example" in image["code_file"] for image in event["debug_meta"]["images"])

def assert_stacktrace(envelope, inside_exception=False, check_size=False):
    event = envelope.get_event()

    frames = (event["exception"] if inside_exception else event["threads"])["values"][0]["stacktrace"]["frames"]
    assert isinstance(frames, list)

    if check_size:
        assert len(frames) > 0
        assert all(frame["instruction_addr"].startswith("0x") for frame in frames)

def assert_breadcrumb(envelope):
    event = envelope.get_event()

    expected = {
        "type": "http",
        "message": "debug crumb",
        "category": "example!",
        "level": "debug",
    }
    assert any(matches(b, expected) for b in event["breadcrumbs"])

def assert_attachment(envelope):
    expected = { "type": "attachment", "name": "CMakeCache.txt", "filename": "CMakeCache.txt" }
    assert any(matches(item.headers, expected) for item in envelope)

def assert_minidump(envelope):
    expected = { "type": "attachment", "name": "upload_file_minidump", "attachment_type": "event.minidump" }
    assert any(matches(item.headers, expected) for item in envelope)

def assert_event(envelope):
    event = envelope.get_event()
    expected = {
        "level": "info",
        "logger": "my-logger",
        "message": { "formatted":"Hello World!" },
    }
    assert matches(event, expected)

def assert_crash(envelope):
    event = envelope.get_event()
    assert matches(event, { "level": "fatal" })
    # depending on the unwinder, we currently don’t get any stack frames from
    # a `ucontext`
    assert_stacktrace(envelope, inside_exception=True)

def test_capture_stdout(tmp_path):
    # backend does not matter, but we want to keep compile times down
    cmake(tmp_path, ["sentry_example"], {"SENTRY_BACKEND": "none", "SENTRY_TRANSPORT":"none", "BUILD_SHARED_LIBS":"OFF"})

    # on linux we can use `ldd` to check that we don’t link to `libsentry.so`
    if sys.platform == "linux":
        output = subprocess.check_output("ldd sentry_example", cwd=tmp_path, shell=True)
        assert b"libsentry.so" not in output

    # on windows, we use `sigcheck` to check that the exe is compiled correctly
    if sys.platform == "win32":
        output = subprocess.run("sigcheck sentry_example.exe", cwd=tmp_path, shell=True, stdout=subprocess.PIPE).stdout
        assert (b"32-bit" if os.environ.get("TEST_X86") else b"64-bit") in output
    # similarly, we use `file` on linux
    if sys.platform == "linux":
        output = subprocess.check_output("file sentry_example", cwd=tmp_path, shell=True)
        assert (b"ELF 32-bit" if os.environ.get("TEST_X86") else b"ELF 64-bit") in output

    output = check_output(tmp_path, "sentry_example", ["stdout", "attachment", "capture-event", "add-stacktrace"])
    envelope = Envelope.deserialize(output)

    assert_meta(envelope)
    assert_breadcrumb(envelope)
    assert_attachment(envelope)
    assert_stacktrace(envelope)

    assert_event(envelope)

@pytest.mark.skipif(os.environ.get("ANDROID_API") or os.environ.get("TEST_X86"),
    reason="Android has no default http transport")
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

@pytest.mark.skipif(sys.platform == "win32", reason="no inproc backend on windows")
def test_inproc_enqueue_stdout(tmp_path):
    cmake(tmp_path, ["sentry_example"], {"SENTRY_BACKEND":"inproc", "SENTRY_TRANSPORT":"none"})

    child = run(tmp_path, "sentry_example", ["attachment", "crash"])
    assert child.returncode # well, its a crash after all

    output = check_output(tmp_path, "sentry_example", ["stdout", "no-setup"])
    envelope = Envelope.deserialize(output)

    assert_meta(envelope)
    assert_breadcrumb(envelope)
    assert_attachment(envelope)

    assert_crash(envelope)

@pytest.mark.skipif(sys.platform != "linux", reason="breakpad only supported on linux")
def test_breakpad_enqueue_stdout(tmp_path):
    cmake(tmp_path, ["sentry_example"], {"SENTRY_BACKEND":"breakpad", "SENTRY_TRANSPORT":"none"})

    child = run(tmp_path, "sentry_example", ["attachment", "crash"])
    assert child.returncode # well, its a crash after all

    output = check_output(tmp_path, "sentry_example", ["stdout", "no-setup"])
    envelope = Envelope.deserialize(output)

    assert_meta(envelope)
    assert_breadcrumb(envelope)
    assert_attachment(envelope)

    assert_minidump(envelope)

@pytest.mark.skipif(sys.platform == "linux" or os.environ.get("ANDROID_API"), reason="crashpad not supported on linux")
def test_crashpad_build(tmp_path):
    cmake(tmp_path, ["sentry_example"], {"SENTRY_BACKEND":"crashpad", "SENTRY_TRANSPORT":"none"})
