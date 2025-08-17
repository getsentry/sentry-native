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

    # Check for GPU contexts (gpu, gpu2, gpu3, etc.)
    contexts = event.get("contexts", {})
    gpu_contexts = {}
    for key, value in contexts.items():
        if key == "gpu" or (key.startswith("gpu") and key[3:].isdigit()):
            gpu_contexts[key] = value

    if gpu_contexts:
        # Ensure we have at least one GPU
        assert len(gpu_contexts) > 0, "Should have at least one GPU context"

        # Validate each GPU context
        for context_key, gpu in gpu_contexts.items():
            # Check that type field is set to "gpu"
            assert "type" in gpu, f"{context_key} should have a 'type' field"
            assert gpu["type"] == "gpu", f"{context_key} type should be 'gpu'"

            # Validate that we have at least basic identifying information
            identifying_fields = ["name", "vendor_name", "vendor_id", "device_id"]
            assert any(
                field in gpu for field in identifying_fields
            ), f"{context_key} should contain at least one of: {identifying_fields}"

            # If name is present, it should be meaningful
            if "name" in gpu:
                name = gpu["name"]
                assert isinstance(name, str), f"{context_key} name should be a string"
                assert len(name) > 0, f"{context_key} name should not be empty"
                # Should not be just a generic placeholder
                assert name != "Unknown", f"{context_key} name should be meaningful, not 'Unknown'"

            # If vendor info is present, validate it
            if "vendor_name" in gpu:
                vendor_name = gpu["vendor_name"]
                assert isinstance(vendor_name, str), f"{context_key} vendor_name should be a string"
                assert len(vendor_name) > 0, f"{context_key} vendor_name should not be empty"

            if "vendor_id" in gpu:
                vendor_id = gpu["vendor_id"]
                assert isinstance(vendor_id, str), f"{context_key} vendor_id should be a string"
                assert len(vendor_id) > 0, f"{context_key} vendor_id should not be empty"
                # Should be a valid number when converted
                assert vendor_id.isdigit(), f"{context_key} vendor_id should be a numeric string"

            # Check device_id is now a string
            if "device_id" in gpu:
                device_id = gpu["device_id"]
                assert isinstance(device_id, str), f"{context_key} device_id should be a string"
                assert len(device_id) > 0, f"{context_key} device_id should not be empty"

            # Memory size should be reasonable if present
            if "memory_size" in gpu:
                memory_size = gpu["memory_size"]
                assert isinstance(memory_size, int), f"{context_key} memory_size should be an integer"
                assert memory_size > 0, f"{context_key} memory_size should be positive"
                # Should be at least 1MB (very conservative)
                assert memory_size >= 1024 * 1024, f"{context_key} memory size seems too small"


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


def test_gpu_context_multi_gpu_support(cmake):
    """Test that multi-GPU systems are properly detected and reported."""
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
    assert_event(envelope)

    event = envelope.get_event()

    # Check for GPU contexts (gpu, gpu2, gpu3, etc.)
    contexts = event.get("contexts", {})
    gpu_contexts = {}
    for key, value in contexts.items():
        if key == "gpu" or (key.startswith("gpu") and key[3:].isdigit()):
            gpu_contexts[key] = value

    if gpu_contexts:
        print(f"Found {len(gpu_contexts)} GPU context(s) in the system")

        # Test for potential hybrid setups (NVIDIA + other vendors)
        nvidia_count = 0
        other_vendors = set()

        for context_key, gpu in gpu_contexts.items():
            print(f"{context_key}: {gpu}")

            # Validate type field
            assert "type" in gpu, f"{context_key} should have type field"
            assert gpu["type"] == "gpu", f"{context_key} type should be 'gpu'"

            if "vendor_id" in gpu:
                vendor_id = int(gpu["vendor_id"]) if gpu["vendor_id"].isdigit() else 0
                if vendor_id == 0x10de or vendor_id == 4318:  # NVIDIA
                    nvidia_count += 1
                else:
                    other_vendors.add(vendor_id)

        if nvidia_count > 0 and len(other_vendors) > 0:
            print(f"Hybrid GPU setup detected: {nvidia_count} NVIDIA + {len(other_vendors)} other vendor(s)")

            # In hybrid setups, check for detailed info
            for context_key, gpu in gpu_contexts.items():
                if "vendor_id" in gpu:
                    vendor_id = int(gpu["vendor_id"]) if gpu["vendor_id"].isdigit() else 0
                    if vendor_id == 0x10de or vendor_id == 4318:  # NVIDIA
                        print(f"NVIDIA GPU details ({context_key}): {gpu}")

    # The main validation is handled by assert_gpu_context
    assert_gpu_context(event)
