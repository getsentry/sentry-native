import os
import pathlib
import shutil
import subprocess
import sys

import pytest

from . import Envelope
from .assertions import assert_inproc_crash
from .build_config import get_test_executable_cmake_args, get_test_executable_env

fixture_path = pathlib.Path("tests/fixtures/inproc_stress")


def compile_test_program(tmp_path):
    build_dir = tmp_path / "inproc_stress_build"
    build_dir.mkdir(exist_ok=True)

    source_dir = pathlib.Path(__file__).parent.parent.resolve()
    include_dir = source_dir / "include"

    cmake_args = get_test_executable_cmake_args(tmp_path, include_dir)
    cmake_args.append(str(fixture_path.resolve()))

    env = get_test_executable_env()

    subprocess.run(
        ["cmake"] + cmake_args,
        check=True,
        cwd=build_dir,
        env=env,
    )

    subprocess.run(
        ["cmake", "--build", ".", "--parallel"],
        check=True,
        cwd=build_dir,
        env=env,
    )

    if sys.platform == "win32":
        return build_dir / "inproc_stress_test.exe"
    else:
        return build_dir / "inproc_stress_test"


def run_stress_test(tmp_path, test_executable, test_name, database_path=None):
    if database_path is None:
        database_path = tmp_path / ".sentry-native"

    env = os.environ.copy()
    if sys.platform != "win32":
        env["LD_LIBRARY_PATH"] = str(tmp_path) + ":" + env.get("LD_LIBRARY_PATH", "")

    proc = subprocess.Popen(
        [str(test_executable), test_name, str(database_path)],
        cwd=tmp_path,
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    stdout, stderr = proc.communicate(timeout=30)
    return proc.returncode, stdout, stderr


def assert_single_crash_envelope(database_path):
    run_dirs = list(database_path.glob("*.run"))
    assert len(run_dirs) == 1, f"Expected 1 .run directory, found {len(run_dirs)}"

    run_dir = run_dirs[0]
    envelopes = list(run_dir.glob("*.envelope"))
    assert (
        len(envelopes) == 1
    ), f"Expected 1 envelope, found {len(envelopes)}: {envelopes}"

    return envelopes[0]


def assert_crash_marker(database_path):
    assert (database_path / "last_crash").exists(), "Crash marker file missing"


def test_inproc_simple_crash(cmake):
    tmp_path = cmake(
        ["sentry"],
        {"SENTRY_BACKEND": "inproc", "SENTRY_TRANSPORT": "none"},
    )

    test_exe = compile_test_program(tmp_path)
    database_path = tmp_path / ".sentry-native"

    try:
        returncode, stdout, stderr = run_stress_test(
            tmp_path, test_exe, "simple-crash", database_path
        )

        assert returncode != 0, f"Process should have crashed. stderr:\n{stderr}"

        assert (
            "crash has been captured" in stderr
        ), f"Crash not captured. stderr:\n{stderr}"

        assert_crash_marker(database_path)
        envelope_path = assert_single_crash_envelope(database_path)

        with open(envelope_path, "rb") as f:
            envelope = Envelope.deserialize(f.read())
        assert_inproc_crash(envelope)

        event = envelope.get_event()
        assert event is not None, "Event missing from envelope"
        assert "exception" in event, "Exception missing from event"

    finally:
        shutil.rmtree(database_path, ignore_errors=True)


def test_inproc_concurrent_crash(cmake):
    """
    Stress test: multiple threads crash simultaneously.

    This test verifies that:
    1. Only ONE crash envelope is generated (not one per thread)
    2. The envelope is not corrupted by concurrent access
    3. The process terminates cleanly after handling the first crash
    """
    tmp_path = cmake(
        ["sentry"],
        {"SENTRY_BACKEND": "inproc", "SENTRY_TRANSPORT": "none"},
    )

    test_exe = compile_test_program(tmp_path)
    database_path = tmp_path / ".sentry-native"

    try:
        returncode, stdout, stderr = run_stress_test(
            tmp_path, test_exe, "concurrent-crash", database_path
        )

        assert returncode != 0, f"Process should have crashed. stderr:\n{stderr}"
        assert (
            "crash has been captured" in stderr
        ), f"Crash not captured. stderr:\n{stderr}"

        assert_crash_marker(database_path)
        envelope_path = assert_single_crash_envelope(database_path)

        with open(envelope_path, "rb") as f:
            content = f.read()
            envelope = Envelope.deserialize(content)

        assert_inproc_crash(envelope)

        event = envelope.get_event()
        assert event is not None, "Event missing from envelope"
        assert "exception" in event, "Exception missing from event"

        exc = event["exception"]["values"][0]
        assert "type" in exc, "Exception type missing"
        assert "stacktrace" in exc, "Stacktrace missing from exception"

    finally:
        shutil.rmtree(database_path, ignore_errors=True)


def test_inproc_handler_thread_crash(cmake):
    """
    Test fallback when handler thread crashes.

    This test verifies that when the on_crash callback crashes (which runs
    on the handler thread), the signal handler detects this and falls back
    to processing the crash directly in the signal handler context.
    """
    tmp_path = cmake(
        ["sentry"],
        {"SENTRY_BACKEND": "inproc", "SENTRY_TRANSPORT": "none"},
    )

    test_exe = compile_test_program(tmp_path)
    database_path = tmp_path / ".sentry-native"

    try:
        returncode, stdout, stderr = run_stress_test(
            tmp_path, test_exe, "handler-thread-crash", database_path
        )

        assert returncode != 0, f"Process should have crashed. stderr:\n{stderr}"

        assert (
            "on_crash callback about to crash" in stderr
        ), f"on_crash callback didn't run. stderr:\n{stderr}"

        assert (
            "FATAL crash in handler thread" in stderr
        ), f"Handler thread crash not detected. stderr:\n{stderr}"

        assert (
            "crash has been captured" in stderr
        ), f"Crash not captured via fallback. stderr:\n{stderr}"

        assert_crash_marker(database_path)
        envelope_path = assert_single_crash_envelope(database_path)
        with open(envelope_path, "rb") as f:
            envelope = Envelope.deserialize(f.read())
        assert_inproc_crash(envelope)

    finally:
        shutil.rmtree(database_path, ignore_errors=True)


@pytest.mark.parametrize("iteration", range(5))
def test_inproc_concurrent_crash_repeated(cmake, iteration):
    tmp_path = cmake(
        ["sentry"],
        {"SENTRY_BACKEND": "inproc", "SENTRY_TRANSPORT": "none"},
    )

    test_exe = compile_test_program(tmp_path)
    database_path = tmp_path / f".sentry-native-{iteration}"

    try:
        returncode, stdout, stderr = run_stress_test(
            tmp_path, test_exe, "concurrent-crash", database_path
        )

        assert returncode != 0, f"Iteration {iteration}: Process should have crashed"
        assert (
            "crash has been captured" in stderr
        ), f"Iteration {iteration}: Crash not captured"

        envelope_path = assert_single_crash_envelope(database_path)
        with open(envelope_path, "rb") as f:
            envelope = Envelope.deserialize(f.read())
        assert_inproc_crash(envelope)

    finally:
        shutil.rmtree(database_path, ignore_errors=True)
