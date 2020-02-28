import pytest
import subprocess
import sys
import json
from . import cmake, Envelope

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
        "release": "integration-test-release",
        "user": { "id": 42, "username": "some_name" },
        "transaction": "test-transaction",
        "tags": { "expected-tag": "some value" },
        "extra": { "extra stuff": "some value" },
    }
    assert matches(event, expected)
    assert any("sentry_test_integration" in image["code_file"] for image in event["debug_meta"]["images"])

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
    expected = { "type": "attachment", "name": "CMakeCache.txt" }
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
    # we have an exception, with at least an empty stacktrace
    assert isinstance(event["exception"]["values"][0]["stacktrace"]["frames"], list)

test_exe = "./sentry_test_integration" if sys.platform != "win32" else "sentry_test_integration.exe"

def test_capture_stdout(tmp_path):
    # backend does not matter, but we want to keep compile times down
    cmake(tmp_path, ["sentry_test_integration"], ["SENTRY_BACKEND=none"])

    output = subprocess.check_output([test_exe, "stdout", "attachment", "capture-event"], cwd=tmp_path)
    envelope = Envelope.deserialize(output)

    assert_meta(envelope)
    assert_breadcrumb(envelope)
    assert_attachment(envelope)

    assert_event(envelope)

@pytest.mark.skipif(sys.platform == "win32", reason="no inproc backend on windows")
def test_inproc_enqueue_stdout(tmp_path):
    cmake(tmp_path, ["sentry_test_integration"], ["SENTRY_BACKEND=inproc"])

    child = subprocess.run([test_exe, "attachment", "crash"], cwd=tmp_path)
    assert child.returncode # well, its a crash after all

    output = subprocess.check_output([test_exe, "stdout", "no-setup"], cwd=tmp_path)
    envelope = Envelope.deserialize(output)

    assert_meta(envelope)
    assert_breadcrumb(envelope)
    assert_attachment(envelope)

    assert_crash(envelope)
