import pytest
import subprocess
import sys
import os
from . import cmake, check_output, run, Envelope
from .assertions import assert_attachment, assert_meta, assert_breadcrumb, assert_stacktrace, assert_event, assert_crash, assert_minidump

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
