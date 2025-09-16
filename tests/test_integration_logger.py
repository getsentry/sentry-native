import shutil
import subprocess
import sys

import pytest

from . import run
from .conditions import has_breakpad, has_crashpad


def _run_logger_crash_test(backend, cmake, logger_option):
    """Helper function to run logger crash tests with the specified logger option.

    Args:
        backend: The sentry backend to use (inproc, breakpad, crashpad)
        cmake: The cmake fixture
        logger_option: Either 'enable-logger-when-crashed' or 'disable-logger-when-crashed'

    Returns:
        tuple: (output, parsed_data) where output is the raw subprocess output
               and parsed_data is the parsed logging data
    """
    tmp_path = cmake(
        ["sentry_example"], {"SENTRY_BACKEND": backend, "SENTRY_TRANSPORT": "none"}
    )

    # Make sure we are isolated from previous runs
    shutil.rmtree(tmp_path / ".sentry-native", ignore_errors=True)

    # Run the example with the specified logger option - expect it to crash
    child = run(
        tmp_path,
        "sentry_example",
        [
            logger_option,  # enable or disable logging during crash
            "log",  # Enable debug logging
            "test-logger",  # Use our custom test logger
            "test-logger-before-crash",  # Log before crash
            "assert",  # Trigger crash
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )

    # Process should have crashed (non-zero exit code)
    assert (
        child.returncode
    ), f"Expected crash but process completed normally. Output: {child.stdout.decode('utf-8', errors='replace')}"

    output = child.stdout.decode("utf-8", errors="replace")

    # Parse the output to check logging behavior
    parsed_data = parse_logger_output(output)

    return parsed_data


def parse_logger_output(output):
    lines = output.split("\n")

    parsed_data = {
        "pre_crash_log_completed": False,
        "sentry_logs": [],
        "logs_after_pre_crash": [],
    }

    pre_crash_completed = False

    for line in lines:
        if line.strip() == "pre-crash-log-message":
            parsed_data["pre_crash_log_completed"] = True
            pre_crash_completed = True
        elif line.startswith("SENTRY_LOG:"):
            log_message = line[len("SENTRY_LOG:") :].strip()
            parsed_data["sentry_logs"].append(log_message)

            # Track logs that occur after the pre-crash marker
            if pre_crash_completed:
                parsed_data["logs_after_pre_crash"].append(log_message)

    return parsed_data


@pytest.mark.parametrize(
    "backend",
    [
        "inproc",
        pytest.param(
            "breakpad",
            marks=pytest.mark.skipif(
                not has_breakpad, reason="breakpad backend not available"
            ),
        ),
        pytest.param(
            "crashpad",
            marks=pytest.mark.skipif(
                not has_crashpad, reason="crashpad backend not available"
            ),
        ),
    ],
)
def test_logger_enabled_when_crashed(backend, cmake):
    """Test that logging works during crash handling when enabled (default behavior)."""
    data = _run_logger_crash_test(backend, cmake, "enable-logger-when-crashed")

    # Verify that pre-crash logging worked
    assert data["pre_crash_log_completed"], "Pre-crash log marker should be present"
    assert len(data["sentry_logs"]) > 0, "Should have some SENTRY_LOG messages"

    # When logging is enabled, we should see logs after the pre-crash marker
    # Only check this on Linux, as other platforms don't reliably log during crash
    if sys.platform == "linux":
        assert (
            len(data["logs_after_pre_crash"]) > 0
        ), "Should have SENTRY_LOG messages after crash when logging is enabled"


@pytest.mark.parametrize(
    "backend",
    [
        "inproc",
        pytest.param(
            "breakpad",
            marks=pytest.mark.skipif(
                not has_breakpad, reason="breakpad backend not available"
            ),
        ),
        pytest.param(
            "crashpad",
            marks=pytest.mark.skipif(
                not has_crashpad, reason="crashpad backend not available"
            ),
        ),
    ],
)
def test_logger_disabled_when_crashed(backend, cmake):
    """Test that logging is disabled during crash handling when the option is set."""
    data = _run_logger_crash_test(backend, cmake, "disable-logger-when-crashed")

    # Verify that pre-crash logging worked
    assert data["pre_crash_log_completed"], "Pre-crash log marker should be present"
    assert len(data["sentry_logs"]) > 0, "Should have some SENTRY_LOG messages"

    # When logging is disabled, we should NOT see any logs after the pre-crash marker
    # The last log should be from before the crash
    assert (
        len(data["logs_after_pre_crash"]) == 0
    ), f"Should have NO SENTRY_LOG messages after crash when logging is disabled, but got: {data['logs_after_pre_crash']}"
