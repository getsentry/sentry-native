import pytest
import subprocess
import sys
import json
from . import cmake

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

# TODO: maybe do some very rudimentary envelope parsing?

def assert_meta(output):
    assert b'"platform":"native"' in output
    assert b'"release":"integration-test-release"' in output
    assert b'"user":{"id":42,"username":"some_name"}' in output
    assert b'"transaction":"test-transaction","tags":{"expected-tag":"some value"},"extra":{"extra stuff":"some value"},' in output
    assert b'/sentry_test_integration","image_addr":"' in output

def assert_breadcrumb(output):
    assert b',"type":"http","message":"debug crumb","category":"example!","level":"debug"}' in output

def assert_attachment(output):
    assert b'{"type":"attachment",' in output
    assert b',"name":"CMakeCache.txt"}' in output

def assert_event(output):
    assert b'"level":"info","logger":"my-logger","message":{"formatted":"Hello World!"}' in output

def assert_crash(output):
    assert b'"level":"fatal","exception":' in output
    assert b'"stacktrace":{"frames":[' in output

test_exe = "./sentry_test_integration" if sys.platform != "win32" else "sentry_test_integration.exe"

def test_capture_stdout(tmp_path):
    # backend does not matter, but we want to keep compile times down
    cmake(tmp_path, ["sentry_test_integration"], ["SENTRY_BACKEND=none"])

    output = subprocess.check_output([test_exe, "stdout", "attachment", "capture-event"], cwd=tmp_path)
    assert_meta(output)
    assert_breadcrumb(output)
    assert_attachment(output)
    assert_event(output)

@pytest.mark.skipif(sys.platform == "win32", reason="no inproc backend on windows")
def test_inproc_enqueue_stdout(tmp_path):
    cmake(tmp_path, ["sentry_test_integration"], ["SENTRY_BACKEND=inproc"])

    child = subprocess.run([test_exe, "attachment", "crash"], cwd=tmp_path)
    assert child.returncode # well, its a crash after all

    output = subprocess.check_output([test_exe, "stdout", "no-setup"], cwd=tmp_path)
    assert_meta(output)
    assert_breadcrumb(output)
    assert_attachment(output)
    assert_crash(output)
