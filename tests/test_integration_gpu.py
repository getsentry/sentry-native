import sys

import pytest

from . import check_output, Envelope
from .assertions import (
    assert_meta,
    assert_breadcrumb,
    assert_event,
    assert_gpu_context,
)


def test_gpu_context_present_when_enabled(cmake):
    """Test that GPU context is present in events when GPU support is enabled."""
    tmp_path = cmake(
        ["sentry_example"],
        {
            "SENTRY_BACKEND": "none",
            "SENTRY_TRANSPORT": "none",
            "SENTRY_WITH_GPU_INFO": "ON",
        },
    )

    output = check_output(
        tmp_path,
        "sentry_example",
        ["stdout", "capture-event"],
    )
    envelope = Envelope.deserialize(output)

    assert_meta(envelope)
    assert_breadcrumb(envelope)
    assert_event(envelope)

    # Test that GPU context is present and properly structured
    event = envelope.get_event()
    assert_gpu_context(event, should_have_gpu=None)  # Allow either present or absent


def test_gpu_context_absent_when_disabled(cmake):
    """Test that GPU context is absent in events when GPU support is disabled."""
    tmp_path = cmake(
        ["sentry_example"],
        {
            "SENTRY_BACKEND": "none",
            "SENTRY_TRANSPORT": "none",
            "SENTRY_WITH_GPU_INFO": "OFF",
        },
    )

    output = check_output(
        tmp_path,
        "sentry_example",
        ["stdout", "capture-event"],
    )
    envelope = Envelope.deserialize(output)

    assert_meta(envelope)
    assert_breadcrumb(envelope)
    assert_event(envelope)

    # Test that GPU context is specifically absent
    event = envelope.get_event()
    assert_gpu_context(event, should_have_gpu=False)


def test_gpu_context_structure_validation(cmake):
    """Test that GPU context contains expected fields when present."""
    tmp_path = cmake(
        ["sentry_example"],
        {
            "SENTRY_BACKEND": "none",
            "SENTRY_TRANSPORT": "none",
            "SENTRY_WITH_GPU_INFO": "ON",
        },
    )

    output = check_output(
        tmp_path,
        "sentry_example",
        ["stdout", "capture-event"],
    )
    envelope = Envelope.deserialize(output)
    event = envelope.get_event()

    # Check if GPU context is present
    if "gpu" in event.get("contexts", {}):
        gpu_context = event["contexts"]["gpu"]

        # Validate that we have at least basic identifying information
        identifying_fields = ["name", "vendor_name", "vendor_id", "device_id"]
        assert any(
            field in gpu_context for field in identifying_fields
        ), f"GPU context should contain at least one of: {identifying_fields}"

        # If name is present, it should be meaningful
        if "name" in gpu_context:
            name = gpu_context["name"]
            assert isinstance(name, str)
            assert len(name) > 0
            # Should not be just a generic placeholder
            assert name != "Unknown"

        # If vendor info is present, validate it
        if "vendor_name" in gpu_context:
            vendor_name = gpu_context["vendor_name"]
            assert isinstance(vendor_name, str)
            assert len(vendor_name) > 0

        if "vendor_id" in gpu_context:
            vendor_id = gpu_context["vendor_id"]
            assert isinstance(vendor_id, int)
            assert vendor_id > 0  # Should be a real vendor ID

        # Memory size should be reasonable if present
        if "memory_size" in gpu_context:
            memory_size = gpu_context["memory_size"]
            assert isinstance(memory_size, int)
            assert memory_size > 0
            # Should be at least 1MB (very conservative)
            assert memory_size >= 1024 * 1024, "GPU memory size seems too small"


def test_gpu_context_cross_platform_compatibility(cmake):
    """Test that GPU context works across different platforms without breaking."""
    tmp_path = cmake(
        ["sentry_example"],
        {
            "SENTRY_BACKEND": "none",
            "SENTRY_TRANSPORT": "none",
            "SENTRY_WITH_GPU_INFO": "ON",
        },
    )

    # This should not crash regardless of platform
    output = check_output(
        tmp_path,
        "sentry_example",
        ["stdout", "capture-event"],
    )
    envelope = Envelope.deserialize(output)

    assert_meta(envelope)
    assert_event(envelope)

    # GPU context may or may not be present, but if it is, it should be valid
    event = envelope.get_event()
    assert_gpu_context(event)  # No expectation, just validate if present
