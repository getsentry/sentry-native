"""
End-to-end integration tests that verify Sentry.io receives crash events.

These tests send real crash events to Sentry and verify they arrive with
the expected structure for all 3 crash reporting modes.

Requires environment variables:
- SENTRY_E2E_DSN: Sentry test project DSN
- SENTRY_E2E_AUTH_TOKEN: Sentry API token with project:read scope
- SENTRY_E2E_ORG: Sentry organization slug
- SENTRY_E2E_PROJECT: Sentry project slug

Skip these tests if env vars not set (for regular CI runs).
"""

import os
import re
import subprocess
import sys
import time
import pytest
import requests

from . import run, check_output
from .conditions import has_native, is_asan, is_kcov

# Skip all tests if E2E env vars not configured
pytestmark = [
    pytest.mark.skipif(
        not os.environ.get("SENTRY_E2E_DSN"),
        reason="E2E tests require SENTRY_E2E_DSN environment variable",
    ),
    pytest.mark.skipif(
        not has_native,
        reason="E2E tests require native backend",
    ),
]

SENTRY_API_BASE = "https://sentry.io/api/0"
POLL_MAX_ATTEMPTS = 20
POLL_INTERVAL = 5  # seconds


def get_sentry_headers():
    """Get authorization headers for Sentry API."""
    token = os.environ.get("SENTRY_E2E_AUTH_TOKEN")
    if not token:
        pytest.skip("SENTRY_E2E_AUTH_TOKEN not set")
    return {"Authorization": f"Bearer {token}"}


def get_sentry_org_project():
    """Get organization and project slugs from environment."""
    org = os.environ.get("SENTRY_E2E_ORG")
    project = os.environ.get("SENTRY_E2E_PROJECT")
    if not org or not project:
        pytest.skip("SENTRY_E2E_ORG and SENTRY_E2E_PROJECT must be set")
    return org, project


def poll_sentry_for_event(test_id, max_attempts=POLL_MAX_ATTEMPTS):
    """
    Poll Sentry API until event with test.id tag appears.

    Args:
        test_id: The unique test ID tag value to search for
        max_attempts: Maximum number of polling attempts

    Returns:
        The full event data from Sentry API

    Raises:
        TimeoutError: If event not found within max attempts
    """
    org, project = get_sentry_org_project()
    headers = get_sentry_headers()

    last_error = None

    print(f"\nWaiting for event with test.id={test_id} to appear in Sentry...")
    print(f"Using org={org}, project={project}")

    for attempt in range(max_attempts):
        try:
            # Use the issues endpoint to find issues by tag
            issues_url = f"{SENTRY_API_BASE}/projects/{org}/{project}/issues/"
            response = requests.get(
                issues_url,
                headers=headers,
                params={"query": f"test.id:{test_id}", "limit": 10},
                timeout=30,
            )

            if response.status_code == 200:
                issues = response.json()
                if issues:
                    # Get the latest event from the first issue
                    issue_id = issues[0]["id"]
                    latest_event_url = (
                        f"{SENTRY_API_BASE}/issues/{issue_id}/events/latest/"
                    )
                    event_response = requests.get(
                        latest_event_url, headers=headers, timeout=30
                    )
                    if event_response.status_code == 200:
                        event = event_response.json()
                        # Verify this event has our test.id tag
                        tags = {t["key"]: t["value"] for t in event.get("tags", [])}
                        if tags.get("test.id") == test_id:
                            print(f"Found event after {attempt + 1} attempts")
                            return event
            elif response.status_code != 200:
                last_error = (
                    f"API returned {response.status_code}: {response.text[:100]}"
                )

        except requests.RequestException as e:
            last_error = str(e)

        time.sleep(POLL_INTERVAL)

    error_msg = f"Event with test.id={test_id} not found after {max_attempts} attempts"
    if last_error:
        error_msg += f". Last error: {last_error}"
    raise TimeoutError(error_msg)


def get_exception_from_event(event):
    """
    Extract exception data from Sentry API event response.

    The API returns exception data in 'entries' array, not directly on event.
    """
    # Check entries array (Sentry API format)
    entries = event.get("entries", [])
    for entry in entries:
        if entry.get("type") == "exception":
            return entry.get("data", {})

    # Fallback: check direct exception field
    if "exception" in event:
        return event["exception"]

    return None


def get_threads_from_event(event):
    """
    Extract threads data from Sentry API event response.

    The API returns threads data in 'entries' array, not directly on event.
    """
    # Check entries array (Sentry API format)
    entries = event.get("entries", [])
    for entry in entries:
        if entry.get("type") == "threads":
            return entry.get("data", {})

    # Fallback: check direct threads field
    if "threads" in event:
        return event["threads"]

    return None


def extract_test_id(output):
    """
    Extract TEST_ID from app output.

    Args:
        output: Bytes output from the test app

    Returns:
        The test ID string (UUID format)

    Raises:
        ValueError: If TEST_ID not found in output
    """
    decoded = output.decode("utf-8", errors="replace")
    match = re.search(r"TEST_ID:([a-f0-9-]{36})", decoded)
    if match:
        return match.group(1)
    raise ValueError(f"TEST_ID not found in output. Output was:\n{decoded[:500]}")


def run_crash_e2e(tmp_path, exe, args, env):
    """
    Run a crash test for E2E, capturing output for test ID extraction.

    Handles ASAN and kcov quirks similar to run_crash in test_integration_native.py.
    """
    if is_asan:
        asan_opts = env.get("ASAN_OPTIONS", "")
        asan_signal_opts = (
            "handle_segv=0:handle_sigbus=0:handle_abort=0:"
            "handle_sigfpe=0:handle_sigill=0:allow_user_segv_handler=1"
        )
        if asan_opts:
            env["ASAN_OPTIONS"] = f"{asan_opts}:{asan_signal_opts}"
        else:
            env["ASAN_OPTIONS"] = asan_signal_opts

    # Use check_output to capture stdout for test ID extraction
    try:
        output = check_output(tmp_path, exe, args, env=env, expect_failure=True)
    except AssertionError:
        if is_kcov:
            # kcov may exit with 0 even on crash, try without expect_failure
            output = check_output(tmp_path, exe, args, env=env, expect_failure=False)
        else:
            raise

    return output


class TestE2ECrashModes:
    """E2E tests for all 3 crash reporting modes against real Sentry."""

    @pytest.fixture(autouse=True)
    def setup(self, cmake):
        """Build the test app and set up DSN."""
        self.tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "native"})
        self.dsn = os.environ["SENTRY_E2E_DSN"]

    def run_crash_and_send(self, mode_args):
        """
        Crash the app with given mode, then restart to send the pending crash.

        Args:
            mode_args: List of crash-mode arguments (e.g., ["crash-mode", "native"])

        Returns:
            The test ID that can be used to find the event in Sentry
        """
        env = dict(os.environ, SENTRY_DSN=self.dsn)

        # Run with crash - capture output for test ID
        # Enable structured logs and capture a log message before crashing
        crash_args = (
            ["log", "e2e-test", "enable-logs", "capture-log"] + mode_args + ["crash"]
        )
        output = run_crash_e2e(self.tmp_path, "sentry_example", crash_args, env=env)
        test_id = extract_test_id(output)

        # Wait for crash daemon to process
        time.sleep(2)

        # Restart to send pending crash (no-setup skips scope setup but still sends)
        run(self.tmp_path, "sentry_example", ["no-setup"], env=env)

        return test_id

    def test_mode_minidump_e2e(self):
        """
        Mode 0 (MINIDUMP): Verify Sentry receives minidump-only crash.

        In minidump mode:
        - Minidump is sent as attachment
        - Server processes minidump for stacktrace
        - No client-side stackwalking
        - Multiple threads captured in minidump
        """
        test_id = self.run_crash_and_send(["crash-mode", "minidump"])

        event = poll_sentry_for_event(test_id)

        # Verify basic event structure
        assert (
            event["platform"] == "native"
        ), f"Expected platform=native, got {event.get('platform')}"

        # Get exception data (API returns it in 'entries' array)
        exception_data = get_exception_from_event(event)
        assert exception_data is not None, "Event should have exception data"
        assert "values" in exception_data, "Exception should have values"

        # Mode 0: Server processes minidump, mechanism type indicates minidump processing
        exc = exception_data["values"][0]
        assert "mechanism" in exc, "Exception should have mechanism"
        # Mechanism type will be 'minidump' when Sentry processes the minidump
        assert exc["mechanism"]["type"] in [
            "minidump",
            "signalhandler",
        ], f"Unexpected mechanism type: {exc['mechanism']['type']}"

        # Verify stacktrace (server-processed from minidump)
        assert (
            "stacktrace" in exc
        ), "Minidump mode should have stacktrace (server-processed)"
        frames = exc["stacktrace"]["frames"]
        assert (
            len(frames) >= 3
        ), f"Minidump mode should have stacktrace (>= 3 frames), got {len(frames)} frames"

        # Verify threads are captured (from minidump)
        threads_data = get_threads_from_event(event)
        assert threads_data is not None, "Minidump mode should have threads data"
        assert "values" in threads_data, "Threads should have values"
        thread_count = len(threads_data["values"])
        assert (
            thread_count >= 1
        ), f"Minidump mode should capture threads (>= 1), got {thread_count}"

    def test_mode_native_e2e(self):
        """
        Mode 1 (NATIVE): Verify Sentry receives native stacktrace, no minidump.

        In native mode:
        - Client-side stackwalking produces stacktrace
        - debug_meta with images is included
        - No minidump attachment
        - Multiple threads are captured
        """
        test_id = self.run_crash_and_send(["crash-mode", "native"])

        event = poll_sentry_for_event(test_id)

        # Verify native stacktrace
        assert (
            event["platform"] == "native"
        ), f"Expected platform=native, got {event.get('platform')}"

        # Get exception data (API returns it in 'entries' array)
        exception_data = get_exception_from_event(event)
        assert exception_data is not None, "Event should have exception data"
        assert "values" in exception_data, "Exception should have values"

        exc = exception_data["values"][0]
        assert "stacktrace" in exc, "Exception should have stacktrace (native mode)"
        frames = exc["stacktrace"]["frames"]
        # Native mode should capture a meaningful stacktrace
        assert (
            len(frames) >= 3
        ), f"Native mode should have stacktrace (>= 3 frames), got {len(frames)} frames"

        # Mode 1: Mechanism should be signalhandler (not minidump)
        assert (
            exc["mechanism"]["type"] == "signalhandler"
        ), f"Native mode should use signalhandler mechanism, got {exc['mechanism']['type']}"

        # Verify threads are captured (native mode includes threads in event)
        threads_data = get_threads_from_event(event)
        assert threads_data is not None, "Native mode should have threads data"
        assert "values" in threads_data, "Threads should have values"
        thread_count = len(threads_data["values"])
        assert (
            thread_count >= 1
        ), f"Native mode should capture threads (>= 1), got {thread_count}"

    def test_mode_native_with_minidump_e2e(self):
        """
        Mode 2 (NATIVE_WITH_MINIDUMP): Verify Sentry receives both native stacktrace AND minidump.

        In native-with-minidump mode (default):
        - Client-side stackwalking produces stacktrace
        - debug_meta with images is included
        - Minidump is also attached for additional debugging
        - Multiple threads are captured (from minidump)
        """
        test_id = self.run_crash_and_send(["crash-mode", "native-with-minidump"])

        event = poll_sentry_for_event(test_id)

        # Verify native stacktrace
        assert (
            event["platform"] == "native"
        ), f"Expected platform=native, got {event.get('platform')}"

        # Get exception data (API returns it in 'entries' array)
        exception_data = get_exception_from_event(event)
        assert exception_data is not None, "Event should have exception data"
        assert "values" in exception_data, "Exception should have values"

        exc = exception_data["values"][0]
        assert (
            "stacktrace" in exc
        ), "Exception should have stacktrace (native-with-minidump mode)"
        frames = exc["stacktrace"]["frames"]
        # Native-with-minidump mode should capture a meaningful stacktrace
        assert (
            len(frames) >= 3
        ), f"Native-with-minidump mode should have stacktrace (>= 3 frames), got {len(frames)} frames"

        # Mode 2: Mechanism can be either signalhandler or minidump
        # (Sentry may process the attached minidump and use that mechanism)
        assert exc["mechanism"]["type"] in [
            "signalhandler",
            "minidump",
        ], f"Unexpected mechanism type: {exc['mechanism']['type']}"

        # Verify threads are captured (from minidump in this mode)
        threads_data = get_threads_from_event(event)
        assert (
            threads_data is not None
        ), "Native-with-minidump mode should have threads data"
        assert "values" in threads_data, "Threads should have values"
        thread_count = len(threads_data["values"])
        assert (
            thread_count >= 1
        ), f"Native-with-minidump mode should capture threads (>= 1), got {thread_count}"

    def test_default_mode_is_native_with_minidump_e2e(self):
        """
        Verify that not specifying a mode uses NATIVE_WITH_MINIDUMP (the default).

        This ensures backward compatibility - the default mode should be the
        most feature-rich option with full stacktrace and multiple threads.
        """
        test_id = self.run_crash_and_send([])  # No crash-mode argument

        event = poll_sentry_for_event(test_id)

        # Default should behave like native-with-minidump
        assert event["platform"] == "native"

        # Get exception data (API returns it in 'entries' array)
        exception_data = get_exception_from_event(event)
        assert exception_data is not None, "Event should have exception data"
        assert "values" in exception_data, "Exception should have values"

        exc = exception_data["values"][0]
        assert "stacktrace" in exc, "Default mode should have native stacktrace"
        frames = exc["stacktrace"]["frames"]
        # Default mode should capture a meaningful stacktrace
        assert (
            len(frames) >= 3
        ), f"Default mode should have stacktrace (>= 3 frames), got {len(frames)} frames"

        # Verify threads are captured (from minidump in default mode)
        threads_data = get_threads_from_event(event)
        assert threads_data is not None, "Default mode should have threads data"
        assert "values" in threads_data, "Threads should have values"
        thread_count = len(threads_data["values"])
        assert (
            thread_count >= 1
        ), f"Default mode should capture threads (>= 1), got {thread_count}"
