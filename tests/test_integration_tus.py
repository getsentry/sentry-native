import os

import pytest

from . import (
    make_dsn,
    run,
    Envelope,
    SENTRY_VERSION,
)
from .conditions import has_crashpad, has_http, is_qemu

pytestmark = pytest.mark.skipif(not has_http, reason="tests need http")

# fmt: off
auth_header = (
    f"Sentry sentry_key=uiaeosnrtdy, sentry_version=7, sentry_client=sentry.native/{SENTRY_VERSION}"
)
# fmt: on


@pytest.mark.skipif(
    not has_crashpad or is_qemu, reason="crashpad backend not available"
)
def test_tus_crash_crashpad(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "crashpad"})

    upload_uri = "/api/123456/upload/abc123def456789/"
    upload_qs = "length=104857600&signature=xyz"
    location = httpserver.url_for(upload_uri) + "?" + upload_qs

    httpserver.expect_oneshot_request(
        "/api/123456/upload/",
        headers={"tus-resumable": "1.0.0"},
    ).respond_with_data("OK", status=201, headers={"Location": location})

    httpserver.expect_oneshot_request(
        upload_uri,
        method="PATCH",
        headers={"tus-resumable": "1.0.0"},
        query_string=upload_qs,
    ).respond_with_data("", status=204)

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    with httpserver.wait(timeout=15) as waiting:
        run(
            tmp_path,
            "sentry_example",
            ["log", "large-attachment", "crashpad-wait-for-upload", "crash"],
            expect_failure=True,
            env=env,
        )
    assert waiting.result

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
    assert int(create_req.headers.get("upload-length")) == 100 * 1024 * 1024
    assert upload_req.headers.get("content-type") == "application/offset+octet-stream"
    assert upload_req.headers.get("upload-offset") == "0"

    envelope = Envelope.deserialize(envelope_req.get_data())
    attachment_ref = None
    minidump_item = None
    for item in envelope:
        if item.headers.get("attachment_type") == "event.minidump":
            minidump_item = item
        if (
            item.headers.get("content_type")
            == "application/vnd.sentry.attachment-ref+json"
            and item.headers.get("filename") == ".sentry-large-attachment"
        ):
            if hasattr(item.payload, "json") and "location" in item.payload.json:
                attachment_ref = item

    assert minidump_item is not None
    assert attachment_ref is not None
    assert attachment_ref.payload.json["location"] == location
    assert attachment_ref.headers.get("attachment_length") == 100 * 1024 * 1024
