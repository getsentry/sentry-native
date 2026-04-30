import json
import os

import pytest

from . import (
    make_dsn,
    run,
    Envelope,
    SENTRY_VERSION,
)
from .assertions import assert_attachment
from .conditions import has_breakpad, has_http, is_qemu

pytestmark = pytest.mark.skipif(not has_http, reason="tests need http")

# fmt: off
auth_header = (
    f"Sentry sentry_key=uiaeosnrtdy, sentry_version=7, sentry_client=sentry.native/{SENTRY_VERSION}"
)
# fmt: on


def test_tus_upload(cmake, httpserver):
    tmp_path = cmake(
        ["sentry_example"],
        {"SENTRY_BACKEND": "none"},
    )

    upload_uri = "/api/123456/upload/abc123def456789/"
    upload_qs = "length=104857600&signature=xyz"
    location = httpserver.url_for(upload_uri) + "?" + upload_qs

    # TUS creation request (POST, no body) -> 201 + Location
    httpserver.expect_oneshot_request(
        "/api/123456/upload/",
        headers={"tus-resumable": "1.0.0"},
    ).respond_with_data(
        "OK",
        status=201,
        headers={"Location": location},
    )

    # TUS upload request (PATCH with body) -> 204
    httpserver.expect_oneshot_request(
        upload_uri,
        method="PATCH",
        headers={"tus-resumable": "1.0.0"},
        query_string=upload_qs,
    ).respond_with_data("", status=204)

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup", "attachment", "large-attachment", "capture-event"],
        env=env,
    )

    assert len(httpserver.log) == 3

    # Find the create, upload, and envelope requests
    create_req = None
    upload_req = None
    envelope_req = None
    for entry in httpserver.log:
        req = entry[0]
        if req.path == "/api/123456/upload/" and req.method == "POST":
            create_req = req
        elif upload_uri in req.path and req.method == "PATCH":
            upload_req = req
        elif "/envelope/" in req.path:
            envelope_req = req

    assert create_req is not None
    assert upload_req is not None
    assert envelope_req is not None

    # Verify TUS creation request headers
    assert create_req.headers.get("tus-resumable") == "1.0.0"
    upload_length = create_req.headers.get("upload-length")
    assert upload_length is not None
    assert int(upload_length) == 100 * 1024 * 1024

    # Verify TUS upload request headers
    assert upload_req.headers.get("tus-resumable") == "1.0.0"
    assert upload_req.headers.get("content-type") == "application/offset+octet-stream"
    assert upload_req.headers.get("upload-offset") == "0"

    # The envelope contains normal small attachments and the large
    # attachment-ref.
    body = envelope_req.get_data()
    envelope = Envelope.deserialize(body)
    assert_attachment(envelope)

    attachment_ref = None
    for item in envelope:
        if (
            item.headers.get("content_type")
            == "application/vnd.sentry.attachment-ref+json"
        ):
            if hasattr(item.payload, "json") and "location" in item.payload.json:
                attachment_ref = item
                break

    assert attachment_ref is not None
    assert attachment_ref.payload.json["location"] == location
    assert not os.path.isabs(attachment_ref.payload.json["path"])
    assert attachment_ref.headers.get("attachment_length") == 100 * 1024 * 1024

    # large attachment files should be cleaned up after send
    cache_dir = os.path.join(tmp_path, ".sentry-native", "cache")
    if os.path.isdir(cache_dir):
        leftovers = [f for f in os.listdir(cache_dir) if not f.endswith(".envelope")]
        assert leftovers == []


def test_tus_error(cmake, httpserver):
    tmp_path = cmake(
        ["sentry_example"],
        {"SENTRY_BACKEND": "none"},
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

    # TUS create failure drops attachment-refs but still sends the envelope.
    body = envelope_req.get_data()
    envelope = Envelope.deserialize(body)
    for item in envelope:
        assert (
            item.headers.get("content_type")
            != "application/vnd.sentry.attachment-ref+json"
        )

    cache_dir = os.path.join(tmp_path, ".sentry-native", "cache")
    if os.path.isdir(cache_dir):
        leftovers = [f for f in os.listdir(cache_dir) if not f.endswith(".envelope")]
        assert leftovers == []


def test_tus_rate_limit(cmake, httpserver):
    tmp_path = cmake(
        ["sentry_example"],
        {"SENTRY_BACKEND": "none"},
    )

    upload_uri = "/api/123456/upload/abc123def456789/"
    upload_qs = "length=104857600&signature=xyz"
    location = httpserver.url_for(upload_uri) + "?" + upload_qs

    httpserver.expect_oneshot_request(
        "/api/123456/upload/",
        headers={"tus-resumable": "1.0.0"},
    ).respond_with_data(
        "OK",
        status=201,
        headers={"Location": location},
    )

    httpserver.expect_oneshot_request(
        upload_uri,
        method="PATCH",
        headers={"tus-resumable": "1.0.0"},
        query_string=upload_qs,
    ).respond_with_data("", status=204)

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data(
        "OK",
        headers={"X-Sentry-Rate-Limits": "60:error:organization"},
    )

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup", "large-attachment", "capture-multiple"],
        env=env,
    )

    # The first envelope installs an error rate limit. Later events should
    # drop their attachment-ref items without starting more TUS uploads.
    uploads = [
        req
        for req, _resp in httpserver.log
        if req.path == "/api/123456/upload/" and req.method == "POST"
    ]
    patches = [
        req
        for req, _resp in httpserver.log
        if req.path == upload_uri and req.method == "PATCH"
    ]
    envelopes = [
        req for req, _resp in httpserver.log if req.path == "/api/123456/envelope/"
    ]

    assert len(httpserver.log) == 3
    assert len(uploads) == 1
    assert len(patches) == 1
    assert len(envelopes) == 1

    cache_dir = os.path.join(tmp_path, ".sentry-native", "cache")
    if os.path.isdir(cache_dir):
        leftovers = [f for f in os.listdir(cache_dir) if not f.endswith(".envelope")]
        assert leftovers == []


@pytest.mark.parametrize(
    "backend",
    [
        "inproc",
        pytest.param(
            "breakpad",
            marks=pytest.mark.skipif(
                not has_breakpad or is_qemu,
                reason="breakpad backend not available",
            ),
        ),
    ],
)
def test_tus_crash_restart(cmake, httpserver, backend):
    tmp_path = cmake(
        ["sentry_example"],
        {"SENTRY_BACKEND": backend},
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

    # After an in-process crash, the disk transport writes the envelope to
    # the current run directory and the large attachment is cached to
    # <cache>/<event-uuid>-<filename>.
    db_dir = os.path.join(tmp_path, ".sentry-native")
    cache_dir = os.path.join(db_dir, "cache")
    assert os.path.isdir(cache_dir)
    run_dirs = [
        d
        for d in os.listdir(db_dir)
        if d.endswith(".run") and os.path.isdir(os.path.join(db_dir, d))
    ]
    assert len(run_dirs) == 1
    run_dir = os.path.join(db_dir, run_dirs[0])
    envelope_files = [f for f in os.listdir(run_dir) if f.endswith(".envelope")]
    assert len(envelope_files) == 1
    base = envelope_files[0][: -len(".envelope")]
    siblings = [f for f in os.listdir(cache_dir) if f.startswith(base)]
    assert len(siblings) > 0
    att_size = os.path.getsize(os.path.join(cache_dir, siblings[0]))
    assert att_size >= 100 * 1024 * 1024

    upload_uri = "/api/123456/upload/abc123def456789/"
    upload_qs = "length=104857600&signature=xyz"
    location = httpserver.url_for(upload_uri) + "?" + upload_qs

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
        upload_uri,
        method="PATCH",
        headers={"tus-resumable": "1.0.0"},
        query_string=upload_qs,
    ).respond_with_data("", status=204)

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

    create_req = None
    upload_req = None
    envelope_req = None
    for entry in httpserver.log:
        req = entry[0]
        if req.path == "/api/123456/upload/" and req.method == "POST":
            create_req = req
        elif upload_uri in req.path and req.method == "PATCH":
            upload_req = req
        elif "/envelope/" in req.path:
            envelope_req = req

    assert create_req is not None
    assert upload_req is not None
    assert envelope_req is not None

    # Verify TUS creation request headers
    assert create_req.headers.get("tus-resumable") == "1.0.0"
    assert int(create_req.headers.get("upload-length")) == 100 * 1024 * 1024

    # Verify TUS upload request headers
    assert upload_req.headers.get("tus-resumable") == "1.0.0"
    assert upload_req.headers.get("content-type") == "application/offset+octet-stream"
    assert upload_req.headers.get("upload-offset") == "0"

    # The envelope contains the event and the attachment-ref
    body = envelope_req.get_data()
    envelope = Envelope.deserialize(body)
    attachment_ref = None
    for item in envelope:
        if (
            item.headers.get("content_type")
            == "application/vnd.sentry.attachment-ref+json"
        ):
            if hasattr(item.payload, "json") and "location" in item.payload.json:
                attachment_ref = item
                break

    assert attachment_ref is not None
    assert attachment_ref.payload.json["location"] == location
    assert not os.path.isabs(attachment_ref.payload.json["path"])
    assert attachment_ref.headers.get("attachment_length") == 100 * 1024 * 1024

    # Staged sibling files should be cleaned up after successful send.
    leftover_siblings = [f for f in os.listdir(cache_dir) if f.startswith(base)]
    assert leftover_siblings == []
