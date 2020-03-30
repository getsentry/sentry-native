import pytest
import subprocess
import sys
import os
import time
from . import cmake, check_output, run, Envelope
from .conditions import has_inproc, has_breakpad, is_android
from .assertions import (
    assert_attachment,
    assert_meta,
    assert_breadcrumb,
    assert_stacktrace,
    assert_event,
    assert_crash,
    assert_minidump,
    assert_timestamp,
)


def test_capture_stdout(tmp_path):
    # backend does not matter, but we want to keep compile times down
    cmake(
        tmp_path,
        ["sentry_example"],
        {
            "SENTRY_BACKEND": "none",
            "SENTRY_TRANSPORT": "none",
            "BUILD_SHARED_LIBS": "OFF",
        },
    )

    # on linux we can use `ldd` to check that we don’t link to `libsentry.so`
    if sys.platform == "linux":
        output = subprocess.check_output("ldd sentry_example", cwd=tmp_path, shell=True)
        assert b"libsentry.so" not in output

    # on windows, we use `sigcheck` to check that the exe is compiled correctly
    if sys.platform == "win32":
        output = subprocess.run(
            "sigcheck sentry_example.exe",
            cwd=tmp_path,
            shell=True,
            stdout=subprocess.PIPE,
        ).stdout
        assert (b"32-bit" if os.environ.get("TEST_X86") else b"64-bit") in output
    # similarly, we use `file` on linux
    if sys.platform == "linux":
        output = subprocess.check_output(
            "file sentry_example", cwd=tmp_path, shell=True
        )
        assert (
            b"ELF 32-bit" if os.environ.get("TEST_X86") else b"ELF 64-bit"
        ) in output

    output = check_output(
        tmp_path,
        "sentry_example",
        ["stdout", "attachment", "capture-event", "add-stacktrace"],
    )
    envelope = Envelope.deserialize(output)

    assert_meta(envelope)
    assert_breadcrumb(envelope)
    assert_attachment(envelope)
    assert_stacktrace(envelope)

    assert_event(envelope)

def test_multi_process(tmp_path):
    cmake(
        tmp_path,
        ["sentry_example"],
        {"SENTRY_BACKEND": "none", "SENTRY_TRANSPORT": "none"},
    )
    cwd = tmp_path
    exe = "sentry_example"
    cmd = "./{}".format(exe) if sys.platform != "win32" else "{}\\{}.exe".format(cwd, exe)

    child1 = subprocess.Popen([cmd, "sleep"], cwd=cwd)
    time.sleep(0.1)
    child2 = subprocess.Popen([cmd, "sleep"], cwd=cwd)
    time.sleep(0.1)

    # while the processes are running, we expect two runs
    runs = [run for run in os.listdir(os.path.join(cwd,".sentry-native")) if run.endswith(".run")]
    assert len(runs) == 2

    # kill the children
    child1.terminate()
    child2.terminate()

    # and start another process that cleans up the old runs
    run(tmp_path, "sentry_example", [])

    runs = [run for run in os.listdir(os.path.join(cwd,".sentry-native")) if run.endswith(".run")]
    assert len(runs) == 0


@pytest.mark.skipif(not has_inproc, reason="test needs inproc backend")
def test_inproc_crash_stdout(tmp_path):
    cmake(
        tmp_path,
        ["sentry_example"],
        {"SENTRY_BACKEND": "inproc", "SENTRY_TRANSPORT": "none"},
    )

    child = run(tmp_path, "sentry_example", ["attachment", "crash"])
    assert child.returncode  # well, its a crash after all

    output = check_output(tmp_path, "sentry_example", ["stdout", "no-setup"])
    envelope = Envelope.deserialize(output)

    # The crash file should survive a `sentry_init` and should still be there
    # even after restarts. On Android, we can’t look inside the emulator.
    if not is_android:
        with open("{}/.sentry-native/last_crash".format(tmp_path)) as f:
            crash_timestamp = f.read()
        assert_timestamp(crash_timestamp)

    assert_meta(envelope)
    assert_breadcrumb(envelope)
    assert_attachment(envelope)

    assert_crash(envelope)


@pytest.mark.skipif(not has_breakpad, reason="test needs breakpad backend")
def test_breakpad_crash_stdout(tmp_path):
    cmake(
        tmp_path,
        ["sentry_example"],
        {"SENTRY_BACKEND": "breakpad", "SENTRY_TRANSPORT": "none"},
    )

    child = run(tmp_path, "sentry_example", ["attachment", "crash"])
    assert child.returncode  # well, its a crash after all

    with open("{}/.sentry-native/last_crash".format(tmp_path)) as f:
        crash_timestamp = f.read()
    assert_timestamp(crash_timestamp)

    output = check_output(tmp_path, "sentry_example", ["stdout", "no-setup"])
    envelope = Envelope.deserialize(output)

    assert_meta(envelope)
    assert_breadcrumb(envelope)
    assert_attachment(envelope)

    assert_minidump(envelope)
