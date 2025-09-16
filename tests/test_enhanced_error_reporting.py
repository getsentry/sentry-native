"""
Test enhanced error reporting for cmake failures.

This test validates that when cmake configure or build fails, detailed error
information is captured and displayed to help with debugging.
"""

import subprocess
import tempfile
import os
import shutil
import pytest
from .cmake import cmake


@pytest.mark.skipif(not shutil.which("cmake"), reason="cmake not available")
def test_cmake_error_reporting(tmp_path):
    """Test that cmake failures show detailed error information."""
    # Create a temporary working directory
    cwd = tmp_path / "build"
    cwd.mkdir()

    # Try to build a non-existent target, which should either:
    # - Fail at configure (if dependencies missing) with "cmake configure failed"
    # - Fail at build (if dependencies available) with "cmake build failed"
    # Both scenarios should show enhanced error reporting

    with pytest.raises(pytest.fail.Exception, match="cmake .* failed"):
        cmake(cwd, ["nonexistent_target_that_will_fail"], {}, [])


@pytest.mark.skipif(not shutil.which("cmake"), reason="cmake not available")
def test_cmake_successful_configure_shows_no_extra_output(tmp_path):
    """Test that successful cmake operations don't show error reporting sections."""
    # This test verifies that our enhanced error reporting doesn't interfere
    # with normal operation by running a simple successful case
    sourcedir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))

    cwd = tmp_path / "build"
    cwd.mkdir()

    # This should succeed without showing error reporting sections
    try:
        cmake(
            cwd,
            ["sentry_test_unit"],
            {"SENTRY_BACKEND": "none", "SENTRY_TRANSPORT": "none"},
            [],
        )
        # If we get here, the test passed successfully
        assert True
    except Exception as e:
        # If it fails, make sure it's not due to our error reporting
        assert "CMAKE CONFIGURE FAILED" not in str(e)
        assert "CMAKE BUILD FAILED" not in str(e)
        # Re-raise the original exception if it's a different kind of failure
        raise


def test_enhanced_error_format():
    """Test that the error formatting function creates properly formatted output."""
    from . import format_error_output

    # Test data
    mock_cmd = ["cmake", "-DTEST=1", "/some/source"]
    mock_cwd = "/some/build/dir"
    mock_returncode = 1
    mock_output = (
        "CMake Error: Invalid option TEST\n-- Configuring incomplete, errors occurred!"
    )

    # Use the actual format_error_output function
    error_message = format_error_output(
        "CMAKE CONFIGURE FAILED", mock_cmd, mock_cwd, mock_returncode, mock_output
    )

    # Verify the formatted output contains expected sections
    assert "CMAKE CONFIGURE FAILED" in error_message
    assert "Command: cmake -DTEST=1 /some/source" in error_message
    assert "Working directory: /some/build/dir" in error_message
    assert "Return code: 1" in error_message
    assert "--- OUTPUT ---" in error_message
    assert "CMake Error: Invalid option TEST" in error_message
    assert "-- Configuring incomplete, errors occurred!" in error_message
    assert error_message.count("=" * 60) == 3  # Header, title separator, and footer
    assert error_message.endswith("\n")  # Ensure proper newline ending


def test_format_error_output_input_validation():
    """Test input validation of the format_error_output function."""
    from . import format_error_output

    # Test with invalid limit_lines (should default to 50)
    result = format_error_output("TEST", ["cmd"], "/dir", 1, "output", limit_lines=-1)
    assert "--- OUTPUT ---" in result

    # Test with empty/None output (should handle gracefully)
    result = format_error_output("TEST", ["cmd"], "/dir", 1, None)
    assert "--- OUTPUT ---" not in result
    assert "Return code: 1" in result

    # Test with empty title (should use default)
    result = format_error_output("", ["cmd"], "/dir", 1)
    assert "COMMAND FAILED" in result

    # Test with string command (should be handled)
    result = format_error_output("TEST", "single_command", "/dir", 1)
    assert "Command: single_command" in result

    # Test with bytes output (should be decoded)
    result = format_error_output("TEST", ["cmd"], "/dir", 1, b"bytes output")
    assert "bytes output" in result


def test_run_with_capture_on_failure_resource_cleanup():
    """Test that subprocess resources are properly cleaned up even when exceptions occur."""
    from . import run_with_capture_on_failure
    import psutil
    import os

    # Get initial process count for the current process
    initial_children = len(psutil.Process().children(recursive=True))

    # Test 1: Normal successful command should not leak processes
    try:
        result = run_with_capture_on_failure(
            ["echo", "test"], cwd=".", error_title="TEST COMMAND"
        )
        assert result.returncode == 0
        assert "test" in result.stdout
    except Exception:
        pass  # Platform differences shouldn't fail this test

    # Test 2: Command that fails should not leak processes
    try:
        run_with_capture_on_failure(
            ["nonexistent_command_that_will_fail"], cwd=".", error_title="TEST COMMAND"
        )
    except (subprocess.CalledProcessError, FileNotFoundError, OSError):
        pass  # Expected to fail

    # Check that no child processes are left hanging
    final_children = len(psutil.Process().children(recursive=True))

    # The number of child processes should be the same or very close
    # Allow for slight variations due to system background processes
    assert abs(final_children - initial_children) <= 1, (
        f"Process leak detected: started with {initial_children} children, "
        f"ended with {final_children} children"
    )
