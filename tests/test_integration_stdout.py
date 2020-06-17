import pytest
import subprocess
import sys
import os
import time
import json
from . import cmake, check_output, run, Envelope
from .conditions import has_inproc, has_breakpad, has_files
from .assertions import (
    assert_attachment,
    assert_meta,
    assert_breadcrumb,
    assert_stacktrace,
    assert_event,
    assert_crash,
    assert_minidump,
    assert_timestamp,
    assert_session,
)


def test_static_lib(cmake):
    tmp_path = cmake(
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


def test_capture_stdout(cmake):
    tmp_path = cmake(
        ["sentry_example"], {"SENTRY_BACKEND": "none", "SENTRY_TRANSPORT": "none",},
    )

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


@pytest.mark.skipif(not has_files, reason="test needs a local filesystem")
def test_multi_process(cmake):
    # NOTE: It would have been nice to do *everything* in a unicode-named
    # directory, but apparently cmake does not like that either.
    tmp_path = cmake(
        ["sentry_example"], {"SENTRY_BACKEND": "none", "SENTRY_TRANSPORT": "none"},
    )

    cwd = tmp_path.joinpath("unicode ❤️ Юля")
    cwd.mkdir()
    exe = "sentry_example"
    cmd = (
        "../{}".format(exe)
        if sys.platform != "win32"
        else "{}\\{}.exe".format(tmp_path, exe)
    )

    child1 = subprocess.Popen([cmd, "sleep"], cwd=cwd)
    child2 = subprocess.Popen([cmd, "sleep"], cwd=cwd)
    time.sleep(0.5)

    # while the processes are running, we expect two runs
    runs = [
        run
        for run in os.listdir(os.path.join(cwd, ".sentry-native"))
        if run.endswith(".run")
    ]
    assert len(runs) == 2

    # kill the children
    child1.terminate()
    child2.terminate()
    child1.wait()
    child2.wait()

    # and start another process that cleans up the old runs
    subprocess.run([cmd], cwd=cwd)

    runs = [
        run
        for run in os.listdir(os.path.join(cwd, ".sentry-native"))
        if run.endswith(".run") or run.endswith(".lock")
    ]
    assert len(runs) == 0


@pytest.mark.skipif(not has_files, reason="test needs a local filesystem")
def test_abnormal_session(cmake):
    tmp_path = cmake(
        ["sentry_example"], {"SENTRY_BACKEND": "none", "SENTRY_TRANSPORT": "none"},
    )

    # create a bogus session file
    db_dir = tmp_path.joinpath(".sentry-native")
    db_dir.mkdir(exist_ok=True)
    run_dir = db_dir.joinpath("foobar.run")
    run_dir.mkdir()
    with open(run_dir.joinpath("session.json"), "w") as session_file:
        json.dump(
            {
                "sid": "00000000-0000-0000-0000-000000000000",
                "did": "42",
                "status": "started",
                "errors": 0,
                "started": "2020-06-02T10:04:53.680Z",
                "duration": 10,
                "attrs": {
                    "release": "test-example-release",
                    "environment": "development",
                },
            },
            session_file,
        )

    output = check_output(tmp_path, "sentry_example", ["stdout", "no-setup"])
    envelope = Envelope.deserialize(output)

    assert_session(envelope, {"status": "abnormal", "errors": 0, "duration": 10})


@pytest.mark.skipif(not has_inproc, reason="test needs inproc backend")
def test_inproc_crash_stdout(cmake):
    tmp_path = cmake(
        ["sentry_example"], {"SENTRY_BACKEND": "inproc", "SENTRY_TRANSPORT": "none"},
    )

    child = run(tmp_path, "sentry_example", ["attachment", "crash"])
    assert child.returncode  # well, its a crash after all

    output = check_output(tmp_path, "sentry_example", ["stdout", "no-setup"])
    envelope = Envelope.deserialize(output)

    # The crash file should survive a `sentry_init` and should still be there
    # even after restarts.
    if has_files:
        with open("{}/.sentry-native/last_crash".format(tmp_path)) as f:
            crash_timestamp = f.read()
        assert_timestamp(crash_timestamp)

    assert_meta(envelope)
    assert_breadcrumb(envelope)
    assert_attachment(envelope)

    assert_crash(envelope)


@pytest.mark.skipif(not has_breakpad, reason="test needs breakpad backend")
def test_breakpad_crash_stdout(cmake):
    tmp_path = cmake(
        ["sentry_example"], {"SENTRY_BACKEND": "breakpad", "SENTRY_TRANSPORT": "none"},
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
