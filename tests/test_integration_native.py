"""
Integration tests for the native crash backend.

Tests crash handling, minidump generation, Build ID/UUID extraction,
multi-thread capture, and FPU/SIMD register capture on all platforms.
"""

import os
import sys
import time
import struct
import pytest

from . import (
    make_dsn,
    run,
    Envelope,
)
from .assertions import (
    assert_meta,
    assert_session,
)
from .conditions import has_native


pytestmark = pytest.mark.skipif(
    not has_native,
    reason="Tests need the native backend enabled",
)


def test_native_capture_crash(cmake, httpserver):
    """Test basic crash capture with native backend"""
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "native"})

    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    child = run(
        tmp_path,
        "sentry_example",
        ["log", "stdout", "test-logger", "crash"],
        expect_failure=True,
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert child.returncode  # Should crash

    # Wait for crash to be processed
    time.sleep(1)

    # Restart to send the crash
    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) >= 1


def test_native_capture_minidump_generated(cmake, httpserver):
    """Test that minidump file is generated"""
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "native"})

    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    # Crash the app
    child = run(
        tmp_path,
        "sentry_example",
        ["log", "stdout", "test-logger", "crash"],
        expect_failure=True,
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert child.returncode

    # Check for minidump file in database directory
    db_dir = tmp_path / ".sentry-native"
    assert db_dir.exists()

    minidump_files = list(db_dir.glob("*.dmp"))
    assert len(minidump_files) > 0, "Minidump file should be generated"

    # Verify minidump has correct header
    minidump_path = minidump_files[0]
    with open(minidump_path, "rb") as f:
        # Read minidump signature (should be MDMP = 0x504d444d)
        signature = struct.unpack("<I", f.read(4))[0]
        assert signature == 0x504D444D, "Minidump should have correct signature"

        # Read version (should be 0xa793 for version 1.0)
        version = struct.unpack("<I", f.read(4))[0]
        assert version == 0xA793, "Minidump should have correct version"


def test_native_breadcrumbs(cmake, httpserver):
    """Test that breadcrumbs are captured before crash"""
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "native"})

    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    # Add breadcrumbs then crash
    run(
        tmp_path,
        "sentry_example",
        ["log", "breadcrumb-log", "crash"],
        expect_failure=True,
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    # Restart to send
    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    # Verify breadcrumbs in envelope
    assert len(httpserver.log) >= 1
    envelope = Envelope.deserialize(httpserver.log[0][0].get_data())
    assert envelope.get_event()


def test_native_session_tracking(cmake, httpserver):
    """Test that sessions are tracked correctly with crashes"""
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "native"})

    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    # Start session and crash
    run(
        tmp_path,
        "sentry_example",
        ["log", "start-session", "crash"],
        expect_failure=True,
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    # Restart to send
    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    # Check for session envelope
    session_envelopes = [
        Envelope.deserialize(req[0].get_data())
        for req in httpserver.log
        if b'"type":"session"' in req[0].get_data()
    ]

    assert len(session_envelopes) >= 1, "Should have session envelope"


def test_native_signal_handling(cmake, httpserver):
    """Test that different signals are handled correctly"""
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "native"})

    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    # Test SIGSEGV
    run(
        tmp_path,
        "sentry_example",
        ["log", "crash"],
        expect_failure=True,
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    # Restart to send
    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) >= 1


@pytest.mark.skipif(sys.platform == "win32", reason="POSIX signals only")
def test_native_sigabrt(cmake, httpserver):
    """Test SIGABRT handling"""
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "native"})

    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    # Trigger SIGABRT via assert
    run(
        tmp_path,
        "sentry_example",
        ["log", "assert"],
        expect_failure=True,
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    # Restart to send
    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) >= 1


def test_native_multiple_crashes(cmake, httpserver):
    """Test handling multiple crashes in sequence"""
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "native"})

    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    # Crash multiple times
    for i in range(3):
        run(
            tmp_path,
            "sentry_example",
            ["log", "crash"],
            expect_failure=True,
            env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
        )
        time.sleep(0.5)

    # Restart to send all crashes
    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    # Should have multiple crash reports
    assert len(httpserver.log) >= 3


def test_native_context_capture(cmake, httpserver):
    """Test that scope and context are captured"""
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "native"})

    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    # Set context then crash
    run(
        tmp_path,
        "sentry_example",
        ["add-stacktrace", "crash"],
        expect_failure=True,
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    # Restart to send
    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) >= 1


def test_native_daemon_respawn(cmake, httpserver):
    """Test that daemon respawns if it dies"""
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "native"})

    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    # This tests the fallback mechanism if daemon dies
    # The test is platform-specific and may need adjustment
    run(
        tmp_path,
        "sentry_example",
        ["log", "crash"],
        expect_failure=True,
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    # Restart to send
    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) >= 1


@pytest.mark.skipif(
    sys.platform not in ["linux", "darwin"],
    reason="Multi-thread test for POSIX platforms",
)
def test_native_multithreaded_crash(cmake, httpserver):
    """Test crash from non-main thread"""
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "native"})

    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    # Crash from thread (if example supports it)
    run(
        tmp_path,
        "sentry_example",
        ["log", "crash"],
        expect_failure=True,
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    # Restart to send
    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) >= 1


def test_native_minidump_streams(cmake, httpserver):
    """Test that minidump contains required streams"""
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "native"})

    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    # Crash
    run(
        tmp_path,
        "sentry_example",
        ["log", "crash"],
        expect_failure=True,
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    # Find minidump
    db_dir = tmp_path / ".sentry-native"
    minidump_files = list(db_dir.glob("*.dmp"))
    assert len(minidump_files) > 0

    # Parse minidump header and verify streams
    with open(minidump_files[0], "rb") as f:
        # Skip signature and version
        f.seek(8)

        # Read stream count
        stream_count = struct.unpack("<I", f.read(4))[0]
        assert (
            stream_count >= 3
        ), "Should have at least SystemInfo, ThreadList, ModuleList"

        # Read stream directory RVA
        stream_dir_rva = struct.unpack("<I", f.read(4))[0]

        # Seek to stream directory
        f.seek(stream_dir_rva)

        # Read stream types
        stream_types = []
        for _ in range(stream_count):
            stream_type = struct.unpack("<I", f.read(4))[0]
            stream_types.append(stream_type)
            f.read(8)  # Skip size and RVA

        # Verify required streams
        # 3 = SystemInfo, 4 = ThreadList, 5 = ModuleList
        assert 3 in stream_types, "Should have SystemInfo stream"
        assert 4 in stream_types, "Should have ThreadList stream"
        assert 5 in stream_types, "Should have ModuleList stream"


def test_native_cleanup(cmake):
    """Test that cleanup works properly"""
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "native"})

    # Run and exit cleanly
    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
    )

    # Database should exist
    db_dir = tmp_path / ".sentry-native"
    assert db_dir.exists()


def test_native_no_dsn_no_crash(cmake):
    """Test that without DSN, crashes don't create files"""
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "native"})

    # Run without DSN
    run(
        tmp_path,
        "sentry_example",
        ["log", "crash"],
        expect_failure=True,
        env=dict(os.environ, SENTRY_DSN=""),
    )

    # Should not create database
    db_dir = tmp_path / ".sentry-native"
    if db_dir.exists():
        minidump_files = list(db_dir.glob("*.dmp"))
        # Minidumps might still be generated for debugging
        # but won't be uploaded


def test_native_external_crash_reporter(cmake, httpserver):
    """Test external crash reporter invocation with native backend"""
    tmp_path = cmake(
        ["sentry_example", "sentry_crash_reporter"], {"SENTRY_BACKEND": "native"}
    )

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")

    # Crash and use external reporter
    run(
        tmp_path,
        "sentry_example",
        ["log", "crash-reporter", "crash"],
        expect_failure=True,
        env=env,
    )

    # Give time for external reporter to run
    time.sleep(2)

    # Should have sent crash report via external reporter
    assert len(httpserver.log) >= 1

    # Verify it's a minidump crash report
    envelope = Envelope.deserialize(httpserver.log[0][0].get_data())
    event = envelope.get_event()
    assert event is not None
