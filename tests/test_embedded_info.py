import os
import subprocess
import pytest
from . import run


def test_embedded_info_enabled(cmake):
    """Test that version embedding works when enabled"""
    cwd = cmake(
        ["sentry_test_unit"],
        {
            "SENTRY_BACKEND": "none",
            "SENTRY_TRANSPORT": "none",
            "SENTRY_EMBED_INFO": "ON",
            "SENTRY_BUILD_PLATFORM": "test-platform",
            "SENTRY_BUILD_VARIANT": "pytest",
            "SENTRY_BUILD_ID": "test-build-123",
            "SENTRY_EMBED_INFO_ITEMS": "TEST:python;FRAMEWORK:pytest;",
        },
    )

    # Run the embedded info tests
    env = dict(os.environ)

    # Test basic functionality
    run(
        cwd,
        "sentry_test_unit",
        ["--no-summary", "embedded_info_basic"],
        check=True,
        env=env,
    )

    # Test format validation
    run(
        cwd,
        "sentry_test_unit",
        ["--no-summary", "embedded_info_format"],
        check=True,
        env=env,
    )

    # Test version matching
    run(
        cwd,
        "sentry_test_unit",
        ["--no-summary", "embedded_info_sentry_version"],
        check=True,
        env=env,
    )


def test_embedded_info_disabled(cmake):
    """Test that version embedding is properly disabled when OFF"""
    cwd = cmake(
        ["sentry_test_unit"],
        {
            "SENTRY_BACKEND": "none",
            "SENTRY_TRANSPORT": "none",
            "SENTRY_EMBED_INFO": "OFF",
        },
    )

    env = dict(os.environ)

    # Test that disabled test passes
    run(
        cwd,
        "sentry_test_unit",
        ["--no-summary", "embedded_info_disabled"],
        check=True,
        env=env,
    )

    # Basic test should skip when disabled
    run(
        cwd,
        "sentry_test_unit",
        ["--no-summary", "embedded_info_basic"],
        check=True,
        env=env,
    )


def test_embedded_info_binary_inspection(cmake):
    """Test that embedded info appears in the actual binary"""
    cwd = cmake(
        ["sentry"],  # Build the library itself
        {
            "SENTRY_EMBED_INFO": "ON",
            "SENTRY_BUILD_PLATFORM": "binary-test",
            "SENTRY_BUILD_VARIANT": "inspection",
            "SENTRY_BUILD_ID": "bin-test-456",
        },
    )

    # Find the library file
    library_file = None
    for file in os.listdir(cwd):
        if file.startswith("libsentry") and (
            file.endswith(".so") or file.endswith(".dylib") or file.endswith(".dll")
        ):
            library_file = os.path.join(cwd, file)
            break

    if library_file is None:
        pytest.skip("Could not find sentry library file")

    # Use strings command to check embedded content
    try:
        result = subprocess.run(
            ["strings", library_file], capture_output=True, text=True, check=True
        )
        output = result.stdout

        # Verify embedded information is present
        assert "SENTRY_VERSION:" in output
        assert "PLATFORM:binary-test" in output
        assert "VARIANT:inspection" in output
        assert "BUILD:bin-test-456" in output
        assert "CONFIG:" in output
        assert "END" in output

    except (subprocess.CalledProcessError, FileNotFoundError):
        pytest.skip("strings command not available or failed")


def test_embedded_info_custom_items(cmake):
    """Test that custom items are properly embedded"""
    cwd = cmake(
        ["sentry_test_unit"],
        {
            "SENTRY_BACKEND": "none",
            "SENTRY_TRANSPORT": "none",
            "SENTRY_EMBED_INFO": "ON",
            "SENTRY_EMBED_INFO_ITEMS": "CUSTOM1:value1;CUSTOM2:value2;ENGINE:unreal;",
        },
    )

    # Build a simple test to check the embedded string content
    env = dict(os.environ)

    # Run format test which will validate the custom items are present
    run(
        cwd,
        "sentry_test_unit",
        ["--no-summary", "embedded_info_format"],
        check=True,
        env=env,
    )
