import json
import os
import threading

import pytest
from werkzeug.wrappers import Response

from . import (
    make_dsn,
    run,
    Envelope,
    SENTRY_VERSION,
)
from .assertions import (
    assert_attachment,
    assert_gzip_content_encoding,
    assert_gzip_file_header,
)
from .conditions import has_breakpad, has_files, has_http, has_native, is_qemu

pytestmark = [
    pytest.mark.skipif(not has_http, reason="tests need http transport"),
    pytest.mark.skipif(not has_files, reason="tests need local filesystem"),
]

# fmt: off
auth_header = (
    f"Sentry sentry_key=uiaeosnrtdy, sentry_version=7, sentry_client=sentry.native/{SENTRY_VERSION}"
)
# sentry <base64({"attachment_type":"event.attachment"})>
upload_metadata = "sentry eyJhdHRhY2htZW50X3R5cGUiOiJldmVudC5hdHRhY2htZW50In0="
# fmt: on


upload_size = 100 * 1024 * 1024
chunk_size = 25 * 1024 * 1024
chunk_headers = {
    "Upload-Chunk-Granularity": "1048576",
    "Upload-Min-Chunk-Size": "1048576",
    "Upload-Max-Chunk-Size": str(chunk_size),
}


def expect_upload_chunks(
    httpserver,
    upload_uri,
    upload_qs,
    final_location,
    offset=0,
    size=chunk_size,
    compressed=False,
):
    while offset < upload_size:
        length = min(size, upload_size - offset)
        headers = {"Upload-Offset": str(offset + length)}
        if offset + length == upload_size:
            headers["Location"] = final_location
        length_header = (
            {"x-decoded-content-length": str(length)}
            if compressed
            else {"content-length": str(length)}
        )
        httpserver.expect_oneshot_request(
            upload_uri,
            method="PATCH",
            headers={
                "tus-resumable": "1.0.0",
                "upload-offset": str(offset),
                **length_header,
            },
            query_string=upload_qs,
        ).respond_with_data("", status=204, headers=headers)
        offset += length


@pytest.mark.parametrize(
    "granularity, build_args",
    [
        (1, {}),
        (6, {}),
        (None, {}),
        (1, {"SENTRY_TRANSPORT_COMPRESSION": "On"}),
    ],
    ids=["chunked", "rounded", "single", "compressed"],
)
def test_tus_upload(cmake, httpserver, granularity, build_args):
    tmp_path = cmake(
        ["sentry_test_integration"],
        {"SENTRY_BACKEND": "none", **build_args},
    )
    compressed = build_args.get("SENTRY_TRANSPORT_COMPRESSION") == "On"

    upload_uri = "/api/123456/upload/abc123def456789/"
    upload_qs = "length=104857600&signature=xyz"
    location = httpserver.url_for(upload_uri) + "?" + upload_qs
    final_qs = "upload_length=104857600&upload_signature=xyz"
    final_location = httpserver.url_for(upload_uri) + "?" + final_qs

    size = upload_size
    headers = {"Location": location}
    if granularity:
        unit = granularity * 1024 * 1024
        size = chunk_size - chunk_size % unit
        headers.update(chunk_headers)
        headers["Upload-Chunk-Granularity"] = str(unit)
        headers["Upload-Min-Chunk-Size"] = str(unit)
    count = (upload_size + size - 1) // size

    # TUS creation request (POST, no body) -> 201 + Location
    httpserver.expect_oneshot_request(
        "/api/123456/upload/",
        method="POST",
        headers={"tus-resumable": "1.0.0"},
    ).respond_with_data(
        "OK",
        status=201,
        headers=headers,
    )

    expect_upload_chunks(
        httpserver,
        upload_uri,
        upload_qs,
        final_location,
        size=size,
        compressed=compressed,
    )

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_test_integration",
        [
            "log",
            "no-setup",
            "attachment",
            "large-attachment",
            "capture-event",
            # extra time for compressing 100 MiB
            "shutdown-timeout",
            "30000",
        ],
        env=env,
    )

    assert len(httpserver.log) == count + 2

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
    assert create_req.headers.get("upload-metadata") == upload_metadata

    # Verify TUS upload request headers
    assert upload_req.headers.get("tus-resumable") == "1.0.0"
    assert upload_req.headers.get("content-type") == "application/offset+octet-stream"
    assert upload_req.headers.get("upload-offset") == str((count - 1) * size)
    if compressed:
        assert_gzip_content_encoding(upload_req)
        assert_gzip_file_header(upload_req.get_data())

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

    # The placeholder uses the Location of the final PATCH response, not the
    # one from the creation request.
    assert attachment_ref is not None
    assert attachment_ref.payload.json["location"] == final_location
    assert attachment_ref.headers.get("attachment_length") == 100 * 1024 * 1024
    assert attachment_ref.headers.get("attachment_type") == "event.attachment"

    # large attachment files should be cleaned up after send
    cache_dir = os.path.join(tmp_path, ".sentry-native", "cache")
    if os.path.isdir(cache_dir):
        leftovers = [f for f in os.listdir(cache_dir) if not f.endswith(".envelope")]
        assert leftovers == []


def test_tus_error(cmake, httpserver):
    tmp_path = cmake(
        ["sentry_test_integration"],
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
        "sentry_test_integration",
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
        ["sentry_test_integration"],
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
        headers={"Location": location, **chunk_headers},
    )

    expect_upload_chunks(httpserver, upload_uri, upload_qs, location)

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
        "sentry_test_integration",
        [
            "log",
            "no-setup",
            "large-attachment",
            "capture-multiple",
            # extra time for pruning 10x100 MiB cache
            "shutdown-timeout",
            "10000",
        ],
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

    assert len(httpserver.log) == 6
    assert len(uploads) == 1
    assert len(patches) == 4
    assert len(envelopes) == 1

    cache_dir = os.path.join(tmp_path, ".sentry-native", "cache")
    if os.path.isdir(cache_dir):
        leftovers = [f for f in os.listdir(cache_dir) if not f.endswith(".envelope")]
        assert leftovers == []


@pytest.mark.parametrize("chunked", [True, False], ids=["chunked", "legacy"])
def test_tus_resume_after_shutdown(cmake, httpserver, chunked):
    tmp_path = cmake(
        ["sentry_test_integration"],
        {"SENTRY_BACKEND": "none"},
    )

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    upload_uri = "/api/123456/upload/abc123def456789/"
    upload_qs = "length=104857600&signature=xyz"
    location = httpserver.url_for(upload_uri) + "?" + upload_qs
    final_qs = "upload_length=104857600&upload_signature=xyz"
    final_location = httpserver.url_for(upload_uri) + "?" + final_qs
    upload_offset = chunk_size if chunked else upload_size // 2

    httpserver.expect_oneshot_request(
        "/api/123456/upload/",
        headers={"tus-resumable": "1.0.0"},
    ).respond_with_data(
        "OK",
        status=201,
        headers={"Location": location, **(chunk_headers if chunked else {})},
    )

    release_upload = threading.Event()

    def delayed_upload(_req):
        release_upload.wait()
        return Response("", status=204, headers={"Upload-Offset": str(upload_offset)})

    httpserver.expect_oneshot_request(
        upload_uri,
        method="PATCH",
        headers={"tus-resumable": "1.0.0", "upload-offset": "0"},
        query_string=upload_qs,
    ).respond_with_handler(delayed_upload)

    with httpserver.wait(timeout=10):
        try:
            run(
                tmp_path,
                "sentry_test_integration",
                [
                    "log",
                    "no-setup",
                    "large-attachment",
                    "capture-event",
                    "sleep-after-shutdown",
                ],
                env=env,
                timeout=30,
            )
        finally:
            release_upload.set()

    db_dir = os.path.join(tmp_path, ".sentry-native")
    cache_dir = os.path.join(db_dir, "cache")
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
    with open(os.path.join(run_dir, envelope_files[0]), "rb") as f:
        persisted = Envelope.deserialize(f.read())
    ref = next(item for item in persisted if item.headers.get("type") == "attachment")
    assert ref.payload.json.get("chunk_size", 0) == (chunk_size if chunked else 0)
    siblings = [f for f in os.listdir(cache_dir) if f.startswith(base)]
    assert len(siblings) > 0
    resume_marker = b"resumed-body"
    with open(os.path.join(cache_dir, siblings[0]), "r+b") as attachment:
        attachment.seek(upload_offset)
        attachment.write(resume_marker)

    httpserver.clear_all_handlers()

    httpserver.expect_oneshot_request(
        upload_uri,
        method="HEAD",
        headers={"tus-resumable": "1.0.0"},
        query_string=upload_qs,
    ).respond_with_data("", status=204, headers={"Upload-Offset": str(upload_offset)})

    expect_upload_chunks(
        httpserver,
        upload_uri,
        upload_qs,
        final_location,
        offset=upload_offset,
        size=chunk_size if chunked else upload_size,
    )

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    with httpserver.wait(timeout=10):
        run(
            tmp_path,
            "sentry_test_integration",
            [
                "log",
                "no-setup",
                # extra time for resuming 100 MiB upload
                "shutdown-timeout",
                "10000",
            ],
            env=env,
        )

    envelope_reqs = [
        entry[0] for entry in httpserver.log if "/envelope/" in entry[0].path
    ]
    assert len(envelope_reqs) == 1

    envelope = Envelope.deserialize(envelope_reqs[0].get_data())
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
    assert attachment_ref.payload.json["location"] == final_location

    upload_reqs = [
        req
        for req, _resp in httpserver.log
        if req.path == upload_uri and req.method == "PATCH"
    ]
    assert len(upload_reqs) == (4 if chunked else 2)
    assert int(upload_reqs[1].headers["content-length"]) == (
        chunk_size if chunked else upload_size - upload_offset
    )
    assert upload_reqs[1].get_data().startswith(resume_marker)

    leftover_siblings = [f for f in os.listdir(cache_dir) if f.startswith(base)]
    assert leftover_siblings == []


@pytest.mark.parametrize("status", [409, 204], ids=["conflict", "partial"])
def test_tus_resume_after_retry(cmake, httpserver, status):
    tmp_path = cmake(
        ["sentry_test_integration"],
        {"SENTRY_BACKEND": "none"},
    )

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    upload_uri = "/api/123456/upload/abc123def456789/"
    upload_qs = "length=104857600&signature=xyz"
    location = httpserver.url_for(upload_uri) + "?" + upload_qs
    final_qs = "upload_length=104857600&upload_signature=xyz"
    final_location = httpserver.url_for(upload_uri) + "?" + final_qs
    upload_offset = 5 * 1024 * 1024

    httpserver.expect_oneshot_request(
        "/api/123456/upload/",
        headers={"tus-resumable": "1.0.0"},
    ).respond_with_data(
        "OK", status=201, headers={"Location": location, **chunk_headers}
    )

    # The server only received part of the body, reported either as an
    # offset conflict or as a partial Upload-Offset on the 204.
    httpserver.expect_oneshot_request(
        upload_uri,
        method="PATCH",
        headers={"tus-resumable": "1.0.0", "upload-offset": "0"},
        query_string=upload_qs,
    ).respond_with_data(
        "", status=status, headers={"Upload-Offset": str(upload_offset)}
    )

    if status == 204:
        expect_upload_chunks(
            httpserver, upload_uri, upload_qs, final_location, offset=upload_offset
        )
        httpserver.expect_oneshot_request(
            "/api/123456/envelope/", headers={"x-sentry-auth": auth_header}
        ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_test_integration",
        ["log", "no-setup", "http-retry", "large-attachment", "capture-event"],
        env=env,
    )

    if status == 204:
        assert len(httpserver.log) == 7
        envelope_req = httpserver.log[-1][0]
        envelope = Envelope.deserialize(envelope_req.get_data())
        ref = next(
            item for item in envelope if item.headers.get("type") == "attachment"
        )
        assert ref.payload.json["location"] == final_location
        return

    # The envelope is kept for retry instead of being sent without the
    # attachment-ref.
    assert len(httpserver.log) == 2
    cache_dir = os.path.join(tmp_path, ".sentry-native", "cache")
    envelope_files = [f for f in os.listdir(cache_dir) if f.endswith(".envelope")]
    assert len(envelope_files) == 1
    assert "-00-" in envelope_files[0]
    uuid = envelope_files[0][-len(".envelope") - 36 : -len(".envelope")]
    siblings = [f for f in os.listdir(cache_dir) if f.startswith(uuid)]
    assert len(siblings) > 0
    resume_marker = b"resumed-body"
    with open(os.path.join(cache_dir, siblings[0]), "r+b") as attachment:
        attachment.seek(upload_offset)
        attachment.write(resume_marker)

    httpserver.clear_all_handlers()

    httpserver.expect_oneshot_request(
        upload_uri,
        method="HEAD",
        headers={"tus-resumable": "1.0.0"},
        query_string=upload_qs,
    ).respond_with_data("", status=204, headers={"Upload-Offset": str(upload_offset)})

    expect_upload_chunks(
        httpserver, upload_uri, upload_qs, final_location, offset=upload_offset
    )

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    with httpserver.wait(timeout=10):
        run(
            tmp_path,
            "sentry_test_integration",
            [
                "log",
                "no-setup",
                "http-retry",
                # extra time for resuming 100 MiB upload
                "shutdown-timeout",
                "10000",
            ],
            env=env,
        )

    envelope_reqs = [
        entry[0] for entry in httpserver.log if "/envelope/" in entry[0].path
    ]
    assert len(envelope_reqs) == 1

    envelope = Envelope.deserialize(envelope_reqs[0].get_data())
    assert envelope.headers["event_id"] == uuid
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
    assert attachment_ref.payload.json["location"] == final_location

    upload_reqs = [
        req
        for req, _resp in httpserver.log
        if req.path == upload_uri and req.method == "PATCH"
    ]
    assert len(upload_reqs) == 5
    assert int(upload_reqs[1].headers["content-length"]) == chunk_size
    assert upload_reqs[1].get_data().startswith(resume_marker)

    leftovers = [f for f in os.listdir(cache_dir) if f.startswith(uuid)]
    assert leftovers == []
    assert [f for f in os.listdir(cache_dir) if f.endswith(".envelope")] == []


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
        ["sentry_test_integration"],
        {"SENTRY_BACKEND": backend},
    )

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    # First run: crash with large attachment (no server expectations needed)
    run(
        tmp_path,
        "sentry_test_integration",
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
    final_qs = "upload_length=104857600&upload_signature=xyz"
    final_location = httpserver.url_for(upload_uri) + "?" + final_qs

    # Second run: restart picks up crash and uploads via TUS
    httpserver.expect_oneshot_request(
        "/api/123456/upload/",
        headers={"tus-resumable": "1.0.0"},
    ).respond_with_data(
        "OK",
        status=201,
        headers={"Location": location, **chunk_headers},
    )

    expect_upload_chunks(httpserver, upload_uri, upload_qs, final_location)

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_test_integration",
        ["log", "no-setup"],
        env=env,
    )

    assert len(httpserver.log) == 6

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
    assert create_req.headers.get("upload-metadata") == upload_metadata

    # Verify TUS upload request headers
    assert upload_req.headers.get("tus-resumable") == "1.0.0"
    assert upload_req.headers.get("content-type") == "application/offset+octet-stream"
    assert upload_req.headers.get("upload-offset") == str(3 * chunk_size)

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
    assert attachment_ref.payload.json["location"] == final_location
    assert attachment_ref.headers.get("attachment_length") == 100 * 1024 * 1024

    # Staged sibling files should be cleaned up after successful send.
    leftover_siblings = [f for f in os.listdir(cache_dir) if f.startswith(base)]
    assert leftover_siblings == []


@pytest.mark.skipif(not has_native or is_qemu, reason="native backend not available")
def test_tus_crash_native(cmake, httpserver):
    # The native backend's daemon process handles the crash out-of-process: it
    # caches the large attachment, writes the envelope with attachment-ref
    # items, and uploads via TUS itself when the server responds within the
    # crash transport shutdown window.
    tmp_path = cmake(["sentry_test_integration"], {"SENTRY_BACKEND": "native"})

    upload_uri = "/api/123456/upload/abc123def456789/"
    upload_qs = "length=104857600&signature=xyz"
    location = httpserver.url_for(upload_uri) + "?" + upload_qs
    final_qs = "upload_length=104857600&upload_signature=xyz"
    final_location = httpserver.url_for(upload_uri) + "?" + final_qs

    httpserver.expect_oneshot_request(
        "/api/123456/upload/",
        headers={"tus-resumable": "1.0.0"},
    ).respond_with_data(
        "OK", status=201, headers={"Location": location, **chunk_headers}
    )

    expect_upload_chunks(httpserver, upload_uri, upload_qs, final_location)

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    with httpserver.wait(timeout=15) as waiting:
        run(
            tmp_path,
            "sentry_test_integration",
            [
                "log",
                "large-attachment",
                "async-crash-upload",
                "crash",
            ],
            expect_failure=True,
            env=env,
            wait_for_daemon=True,
        )
    assert waiting.result

    create_req = upload_req = envelope_req = None
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
    assert int(create_req.headers.get("upload-length")) == 100 * 1024 * 1024
    assert create_req.headers.get("upload-metadata") == upload_metadata

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
    assert attachment_ref.payload.json["location"] == final_location

    db_dir = os.path.join(tmp_path, ".sentry-native")
    run_dirs = [
        d
        for d in os.listdir(db_dir)
        if d.endswith(".run") and os.path.isdir(os.path.join(db_dir, d))
    ]
    assert len(run_dirs) == 1
    run_dir = os.path.join(db_dir, run_dirs[0])
    assert not [f for f in os.listdir(run_dir) if f.endswith(".envelope")]


@pytest.mark.parametrize("status", [404, 410])
@pytest.mark.parametrize("chunked", [True, False], ids=["chunked", "single"])
def test_tus_recreate_after_head_404(cmake, httpserver, status, chunked):
    tmp_path = cmake(["sentry_test_integration"], {"SENTRY_BACKEND": "none"})
    cache_dir = tmp_path / ".sentry-native" / "cache"
    cache_dir.mkdir(parents=True)
    event_id = "b951e5e8-2b70-473f-91f3-667daf78d3de"
    run_dir = tmp_path / ".sentry-native" / f"{event_id}.run"
    run_dir.mkdir()
    filename = f"{event_id}-attachment.bin"
    with (cache_dir / filename).open("wb") as attachment:
        attachment.truncate(upload_size)

    upload_uri = "/api/123456/upload/recreated/"
    upload_qs = "length=104857600&signature=new"
    location = httpserver.url_for(upload_uri) + "?" + upload_qs
    final_location = httpserver.url_for(upload_uri) + "?upload_signature=final"
    payload = {
        "path": filename,
        "location": "/api/123456/upload/expired/",
        "content_type": "application/octet-stream",
        "chunk_size": 32 * 1024 * 1024,
    }
    data = json.dumps(payload)
    headers = {
        "type": "attachment",
        "content_type": "application/vnd.sentry.attachment-ref+json",
        "filename": "attachment.bin",
        "attachment_length": upload_size,
        "length": len(data),
    }
    (run_dir / f"{event_id}.envelope").write_text(
        json.dumps({"event_id": event_id.replace("-", "")})
        + "\n"
        + json.dumps(headers)
        + "\n"
        + data
    )

    httpserver.expect_oneshot_request(
        "/api/123456/upload/expired/", method="HEAD"
    ).respond_with_data("", status=status)
    httpserver.expect_oneshot_request(
        "/api/123456/upload/", method="POST"
    ).respond_with_data(
        "",
        status=201,
        headers={"Location": location, **(chunk_headers if chunked else {})},
    )
    expect_upload_chunks(
        httpserver,
        upload_uri,
        upload_qs,
        final_location,
        size=chunk_size if chunked else upload_size,
    )
    httpserver.expect_oneshot_request(
        "/api/123456/envelope/", headers={"x-sentry-auth": auth_header}
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_test_integration",
        ["log", "no-setup", "shutdown-timeout", "10000"],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) == (7 if chunked else 4)
    envelope = Envelope.deserialize(httpserver.log[-1][0].get_data())
    ref = next(iter(envelope))
    assert ref.payload.json["location"] == final_location
    assert not list(cache_dir.iterdir())


@pytest.mark.parametrize(
    "offset",
    [None, "0", "-1", "invalid", "26214400", "104857601", "18446744073709551616"],
)
def test_tus_invalid_offset(cmake, httpserver, offset):
    tmp_path = cmake(["sentry_test_integration"], {"SENTRY_BACKEND": "none"})
    upload_uri = "/api/123456/upload/invalid-offset/"
    httpserver.expect_oneshot_request(
        "/api/123456/upload/", method="POST"
    ).respond_with_data(
        "", status=201, headers={"Location": upload_uri, **chunk_headers}
    )
    httpserver.expect_oneshot_request(
        upload_uri, method="PATCH", headers={"upload-offset": "0"}
    ).respond_with_data(
        "",
        status=204,
        headers={"Upload-Offset": str(chunk_size), "Location": "/non-final/"},
    )
    httpserver.expect_oneshot_request(
        upload_uri, method="PATCH", headers={"upload-offset": str(chunk_size)}
    ).respond_with_data(
        "", status=204, headers={"Upload-Offset": offset} if offset is not None else {}
    )

    run(
        tmp_path,
        "sentry_test_integration",
        ["log", "no-setup", "http-retry", "large-attachment", "capture-event"],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) == 3
    cache_dir = tmp_path / ".sentry-native" / "cache"
    envelopes = list(cache_dir.glob("*.envelope"))
    assert len(envelopes) == 1
    envelope = Envelope.deserialize(envelopes[0].read_bytes())
    ref = next(item for item in envelope if item.headers.get("type") == "attachment")
    assert ref.payload.json["location"] == upload_uri
    assert ref.payload.json["chunk_size"] == chunk_size


@pytest.mark.parametrize(
    "has_location", [True, False], ids=["location", "missing-location"]
)
def test_tus_resume_completed_upload(cmake, httpserver, has_location):
    tmp_path = cmake(["sentry_test_integration"], {"SENTRY_BACKEND": "none"})
    cache_dir = tmp_path / ".sentry-native" / "cache"
    cache_dir.mkdir(parents=True)
    event_id = "b951e5e8-2b70-473f-91f3-667daf78d3de"
    run_dir = tmp_path / ".sentry-native" / f"{event_id}.run"
    run_dir.mkdir()
    filename = f"{event_id}-attachment.bin"
    with (cache_dir / filename).open("wb") as attachment:
        attachment.truncate(upload_size)

    upload_uri = "/api/123456/upload/completed/"
    upload_qs = "length=104857600&signature=xyz"
    location = upload_uri + "?" + upload_qs
    final_qs = "upload_length=104857600&upload_signature=xyz"
    final_location = httpserver.url_for(upload_uri) + "?" + final_qs
    payload = {
        "path": filename,
        "location": location,
        "content_type": "application/octet-stream",
        "chunk_size": chunk_size,
    }
    data = json.dumps(payload)
    headers = {
        "type": "attachment",
        "content_type": "application/vnd.sentry.attachment-ref+json",
        "filename": "attachment.bin",
        "attachment_length": upload_size,
        "length": len(data),
    }
    (run_dir / f"{event_id}.envelope").write_text(
        json.dumps({"event_id": event_id.replace("-", "")})
        + "\n"
        + json.dumps(headers)
        + "\n"
        + data
    )

    httpserver.expect_oneshot_request(
        upload_uri, method="HEAD", query_string=upload_qs
    ).respond_with_data("", status=204, headers={"Upload-Offset": str(upload_size)})
    httpserver.expect_oneshot_request(
        upload_uri,
        method="PATCH",
        headers={
            "tus-resumable": "1.0.0",
            "content-type": "application/offset+octet-stream",
            "upload-offset": str(upload_size),
        },
        query_string=upload_qs,
        data=b"",
    ).respond_with_data(
        "",
        status=204,
        headers={
            "Upload-Offset": str(upload_size),
            **({"Location": final_location} if has_location else {}),
        },
    )
    httpserver.expect_oneshot_request(
        "/api/123456/envelope/", headers={"x-sentry-auth": auth_header}
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_test_integration",
        ["log", "no-setup", "http-retry"],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) == (3 if has_location else 2)
    if has_location:
        envelope = Envelope.deserialize(httpserver.log[-1][0].get_data())
        ref = next(iter(envelope))
        assert ref.payload.json["location"] == final_location
        assert not list(cache_dir.iterdir())
    else:
        envelopes = list(cache_dir.glob("*.envelope"))
        assert len(envelopes) == 1
        envelope = Envelope.deserialize(envelopes[0].read_bytes())
        ref = next(iter(envelope))
        assert ref.payload.json["location"] == location
        assert ref.payload.json["chunk_size"] == chunk_size
        assert (cache_dir / filename).is_file()
