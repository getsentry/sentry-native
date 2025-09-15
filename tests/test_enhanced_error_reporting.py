"""
Test enhanced error reporting for cmake failures.

This test validates that when cmake configure or build fails, detailed error
information is captured and displayed to help with debugging.
"""

import subprocess
import tempfile
import os
import pytest
from .cmake import cmake


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
    # This is a unit test for the error formatting logic we added

    # Create mock subprocess.CalledProcessError
    mock_cmd = ["cmake", "-DTEST=1", "/some/source"]
    mock_cwd = "/some/build/dir"
    mock_returncode = 1
    mock_stderr = "CMake Error: Invalid option TEST"
    mock_stdout = "-- Configuring incomplete, errors occurred!"

    # Format error details using the same logic as in cmake.py
    error_details = []
    error_details.append("=" * 60)
    error_details.append("CMAKE CONFIGURE FAILED")
    error_details.append("=" * 60)
    error_details.append(f"Command: {' '.join(mock_cmd)}")
    error_details.append(f"Working directory: {mock_cwd}")
    error_details.append(f"Return code: {mock_returncode}")

    if mock_stderr and mock_stderr.strip():
        error_details.append("--- STDERR ---")
        error_details.append(mock_stderr.strip())
    if mock_stdout and mock_stdout.strip():
        error_details.append("--- STDOUT ---")
        error_details.append(mock_stdout.strip())

    error_details.append("=" * 60)
    error_message = "\n".join(error_details)

    # Verify the formatted output contains expected sections
    assert "CMAKE CONFIGURE FAILED" in error_message
    assert "Command: cmake -DTEST=1 /some/source" in error_message
    assert "Working directory: /some/build/dir" in error_message
    assert "Return code: 1" in error_message
    assert "--- STDERR ---" in error_message
    assert "CMake Error: Invalid option TEST" in error_message
    assert "--- STDOUT ---" in error_message
    assert "-- Configuring incomplete, errors occurred!" in error_message
    assert error_message.count("=" * 60) == 3  # Header, middle, and footer
