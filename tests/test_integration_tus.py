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

    httpserver.expect_request(
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

    assert len(httpserver.log) == 3

    # Find the upload request and envelope requests
    upload_req = None
    envelope_reqs = []
    for entry in httpserver.log:
        req = entry[0]
        if "/upload/" in req.path:
            upload_req = req
        elif "/envelope/" in req.path:
            envelope_reqs.append(req)

    assert upload_req is not None
    assert len(envelope_reqs) == 2

    # Verify TUS upload request headers
    assert upload_req.headers.get("tus-resumable") == "1.0.0"
    assert upload_req.headers.get("content-type") == "application/offset+octet-stream"
    upload_length = upload_req.headers.get("upload-length")
    assert upload_length is not None
    assert int(upload_length) == 100 * 1024 * 1024

    # One envelope has the resolved attachment-refs, the other is the original
    attachment_ref = None
    for envelope_req in envelope_reqs:
        body = envelope_req.get_data()
        envelope = Envelope.deserialize(body)
        for item in envelope:
            if (
                item.headers.get("content_type")
                == "application/vnd.sentry.attachment-ref"
            ):
                if hasattr(item.payload, "json") and "location" in item.payload.json:
                    attachment_ref = item
                    break

    assert attachment_ref is not None
    assert attachment_ref.payload.json["location"] == location
    assert attachment_ref.headers.get("attachment_length") == 100 * 1024 * 1024

    # large attachment files should be cleaned up after send
    cache_dir = os.path.join(tmp_path, ".sentry-native", "cache")
    if os.path.isdir(cache_dir):
        subdirs = [
            d
            for d in os.listdir(cache_dir)
            if os.path.isdir(os.path.join(cache_dir, d))
        ]
        assert subdirs == []


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

    # TUS 404: attachment-refs were cached but not resolved, so envelope has
    # no attachment-ref items
    body = envelope_req.get_data()
    envelope = Envelope.deserialize(body)
    for item in envelope:
        assert (
            item.headers.get("content_type") != "application/vnd.sentry.attachment-ref"
        )


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

    # Verify large attachment was persisted to disk in cache/<uuid>/
    cache_dir = os.path.join(tmp_path, ".sentry-native", "cache")
    assert os.path.isdir(cache_dir)
    att_dirs = [
        d for d in os.listdir(cache_dir) if os.path.isdir(os.path.join(cache_dir, d))
    ]
    assert len(att_dirs) > 0
    att_dir = os.path.join(cache_dir, att_dirs[0])
    att_files = [
        f
        for f in os.listdir(att_dir)
        if not f.endswith(".json") and os.path.isfile(os.path.join(att_dir, f))
    ]
    refs_files = [f for f in os.listdir(att_dir) if f == "refs.json"]
    assert len(att_files) > 0
    assert len(refs_files) > 0
    att_size = os.path.getsize(os.path.join(att_dir, att_files[0]))
    assert att_size >= 100 * 1024 * 1024

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

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=env,
    )

    assert len(httpserver.log) == 3

    upload_req = None
    envelope_reqs = []
    for entry in httpserver.log:
        req = entry[0]
        if "/upload/" in req.path:
            upload_req = req
        elif "/envelope/" in req.path:
            envelope_reqs.append(req)

    assert upload_req is not None
    assert len(envelope_reqs) == 2

    # Verify TUS upload request headers
    assert upload_req.headers.get("tus-resumable") == "1.0.0"
    assert upload_req.headers.get("content-type") == "application/offset+octet-stream"
    assert int(upload_req.headers.get("upload-length")) == 100 * 1024 * 1024

    # One envelope has the resolved attachment-refs, the other is the original
    attachment_ref = None
    for envelope_req in envelope_reqs:
        body = envelope_req.get_data()
        envelope = Envelope.deserialize(body)
        for item in envelope:
            if (
                item.headers.get("content_type")
                == "application/vnd.sentry.attachment-ref"
            ):
                if hasattr(item.payload, "json") and "location" in item.payload.json:
                    attachment_ref = item
                    break

    assert attachment_ref is not None
    assert attachment_ref.payload.json["location"] == location
    assert attachment_ref.headers.get("attachment_length") == 100 * 1024 * 1024

    # Large attachment dirs should be cleaned up after send
    remaining_dirs = [
        d for d in os.listdir(cache_dir) if os.path.isdir(os.path.join(cache_dir, d))
    ]
    assert remaining_dirs == []


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
