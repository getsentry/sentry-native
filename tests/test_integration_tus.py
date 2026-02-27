import json
import os

import pytest

from . import (
    make_dsn,
    run,
    Envelope,
    SENTRY_VERSION,
)
from .conditions import has_http

pytestmark = pytest.mark.skipif(not has_http, reason="tests need http")

# fmt: off
auth_header = (
    f"Sentry sentry_key=uiaeosnrtdy, sentry_version=7, sentry_client=sentry.native/{SENTRY_VERSION}"
)
# fmt: on


def test_tus_upload_large_attachment(cmake, httpserver):
    tmp_path = cmake(
        ["sentry_example"],
        {"SENTRY_BACKEND": "none", "SENTRY_TRANSPORT_COMPRESSION": "Off"},
    )

    location = "/api/123456/upload/abc123def456789/?length=104857600&signature=xyz"

    httpserver.expect_oneshot_request(
        "/api/123456/upload/",
        headers={"tus-resumable": "1.0.0"},
    ).respond_with_data(
        "OK",
        status=201,
        headers={"Location": location},
    )

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup", "large-attachment", "capture-event"],
        env=env,
    )

    assert len(httpserver.log) == 2

    # Find the upload request and envelope request
    upload_req = None
    envelope_req = None
    for entry in httpserver.log:
        req = entry[0]
        if "/upload/" in req.path:
            upload_req = req
        elif "/envelope/" in req.path:
            envelope_req = req

    assert upload_req is not None
    assert envelope_req is not None

    # Verify TUS upload request headers
    assert upload_req.headers.get("tus-resumable") == "1.0.0"
    assert upload_req.headers.get("content-type") == "application/offset+octet-stream"
    upload_length = upload_req.headers.get("upload-length")
    assert upload_length is not None
    assert int(upload_length) == 100 * 1024 * 1024

    # Verify envelope contains attachment_ref with resolved location
    body = envelope_req.get_data()
    envelope = Envelope.deserialize(body)
    attachment_ref = None
    for item in envelope:
        if item.headers.get("content_type") == "application/vnd.sentry.attachment-ref":
            if hasattr(item.payload, "json") and "location" in item.payload.json:
                attachment_ref = item
                break

    assert attachment_ref is not None
    assert attachment_ref.payload.json["location"] == location
    assert attachment_ref.headers.get("attachment_length") == 100 * 1024 * 1024

    # large attachment files should be cleaned up after send
    attachments_dir = os.path.join(tmp_path, ".sentry-native", "attachments")
    assert not os.path.exists(attachments_dir) or os.listdir(attachments_dir) == []


def test_tus_upload_404_disables(cmake, httpserver):
    tmp_path = cmake(
        ["sentry_example"],
        {"SENTRY_BACKEND": "none", "SENTRY_TRANSPORT_COMPRESSION": "Off"},
    )

    httpserver.expect_oneshot_request(
        "/api/123456/upload/",
    ).respond_with_data("Not Found", status=404)

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup", "large-attachment", "capture-event"],
        env=env,
    )

    assert len(httpserver.log) == 2

    # Find the envelope request
    envelope_req = None
    for entry in httpserver.log:
        req = entry[0]
        if "/envelope/" in req.path:
            envelope_req = req

    assert envelope_req is not None

    # Unresolved attachment_ref stays with "path", no "location"
    body = envelope_req.get_data()
    envelope = Envelope.deserialize(body)
    for item in envelope:
        if item.headers.get("content_type") == "application/vnd.sentry.attachment-ref":
            assert "path" in item.payload.json
            assert "location" not in item.payload.json


def test_tus_crash_restart(cmake, httpserver):
    tmp_path = cmake(
        ["sentry_example"],
        {"SENTRY_BACKEND": "inproc", "SENTRY_TRANSPORT_COMPRESSION": "Off"},
    )

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    # First run: crash with large attachment (no server expectations needed)
    run(
        tmp_path,
        "sentry_example",
        ["log", "large-attachment", "crash"],
        expect_failure=True,
        env=env,
    )

    # Verify large attachment was persisted to disk
    attachments_dir = os.path.join(tmp_path, ".sentry-native", "attachments")
    assert os.path.isdir(attachments_dir)
    assert len(os.listdir(attachments_dir)) > 0

    location = "/api/123456/upload/abc123def456789/?length=104857600&signature=xyz"

    # Second run: restart picks up crash and uploads via TUS
    httpserver.expect_oneshot_request(
        "/api/123456/upload/",
        headers={"tus-resumable": "1.0.0"},
    ).respond_with_data(
        "OK",
        status=201,
        headers={"Location": location},
    )

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=env,
    )

    assert len(httpserver.log) == 2

    upload_req = None
    envelope_req = None
    for entry in httpserver.log:
        req = entry[0]
        if "/upload/" in req.path:
            upload_req = req
        elif "/envelope/" in req.path:
            envelope_req = req

    assert upload_req is not None
    assert envelope_req is not None

    # Verify TUS upload request headers
    assert upload_req.headers.get("tus-resumable") == "1.0.0"
    assert upload_req.headers.get("content-type") == "application/offset+octet-stream"
    assert int(upload_req.headers.get("upload-length")) == 100 * 1024 * 1024

    # Verify envelope contains attachment_ref with resolved location
    body = envelope_req.get_data()
    envelope = Envelope.deserialize(body)
    attachment_ref = None
    for item in envelope:
        if item.headers.get("content_type") == "application/vnd.sentry.attachment-ref":
            if hasattr(item.payload, "json") and "location" in item.payload.json:
                attachment_ref = item
                break

    assert attachment_ref is not None
    assert attachment_ref.payload.json["location"] == location
    assert attachment_ref.headers.get("attachment_length") == 100 * 1024 * 1024

    # Large attachment files should be cleaned up after send
    assert not os.path.exists(attachments_dir) or os.listdir(attachments_dir) == []


def test_small_attachment_no_tus(cmake, httpserver):
    tmp_path = cmake(
        ["sentry_example"],
        {"SENTRY_BACKEND": "none", "SENTRY_TRANSPORT_COMPRESSION": "Off"},
    )

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup", "attachment", "capture-event"],
        env=env,
    )

    # Only 1 request - no TUS upload for small attachments
    assert len(httpserver.log) == 1
    req = httpserver.log[0][0]
    assert "/envelope/" in req.path
