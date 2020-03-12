import pytest
import subprocess
import sys
import json
from . import cmake, check_output, run, Envelope

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
#
# * test normal event submission
#   - with http
#   - with stdout

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
            "version": "0.2.0",
            "packages": [
                {
                    "name": "github:getsentry/sentry-native",
                    "version": "0.2.0",
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
    cmake(tmp_path, ["sentry_example"], ["SENTRY_BACKEND=none", "BUILD_SHARED_LIBS=OFF"])

    # on linux we can use `ldd` to check that we don’t link to `libsentry.so`
    if sys.platform == "linux":
        output = subprocess.check_output("ldd sentry_example", cwd=tmp_path, shell=True)
        assert b"libsentry.so" not in output

    output = check_output(tmp_path, "sentry_example", ["stdout", "attachment", "capture-event", "add-stacktrace"])
    envelope = Envelope.deserialize(output)

    assert_meta(envelope)
    assert_breadcrumb(envelope)
    assert_attachment(envelope)
    assert_stacktrace(envelope)

    assert_event(envelope)

@pytest.mark.skipif(sys.platform == "win32", reason="no inproc backend on windows")
def test_inproc_enqueue_stdout(tmp_path):
    cmake(tmp_path, ["sentry_example"], ["SENTRY_BACKEND=inproc"])

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
    cmake(tmp_path, ["sentry_example"], ["SENTRY_BACKEND=breakpad"])

    child = run(tmp_path, "sentry_example", ["attachment", "crash"])
    assert child.returncode # well, its a crash after all

    output = check_output(tmp_path, "sentry_example", ["stdout", "no-setup"])
    envelope = Envelope.deserialize(output)

    assert_meta(envelope)
    assert_breadcrumb(envelope)
    assert_attachment(envelope)

    assert_minidump(envelope)

@pytest.mark.skipif(sys.platform == "linux" or sys.platform == "win32",
    reason="crashpad not supported on linux, building is broken in VS2017")
def test_crashpad_build(tmp_path):
    cmake(tmp_path, ["sentry_example"], ["SENTRY_BACKEND=crashpad"])
