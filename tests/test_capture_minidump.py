import itertools
import json
import os
import time
import uuid

import pytest

from . import make_dsn, run, Envelope
from .assertions import (
    assert_attachment,
    assert_meta,
    assert_breadcrumb,
    assert_stacktrace,
    assert_event,
    assert_exception,
    assert_inproc_crash,
    assert_session,
    assert_user_feedback,
    assert_minidump,
    assert_breakpad_crash,
    assert_gzip_content_encoding,
    assert_gzip_file_header,
)
from .conditions import has_http, has_breakpad, has_files

pytestmark = pytest.mark.skipif(not has_http, reason="tests need http")

# fmt: off
auth_header = (
    "Sentry sentry_key=uiaeosnrtdy, sentry_version=7, sentry_client=sentry.native/0.7.10"
)
# fmt: on

def test_capture_minidump(cmake, httpserver, build_args):
    build_args.update({"SENTRY_BACKEND": "none"})
    tmp_path = cmake(["sentry_example"], build_args)

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver), SENTRY_RELEASE="🤮🚀")

    run(
        tmp_path,
        "sentry_example",
        ["log", "release-env", "capture-minidump"],
        check=True,
        env=env,
    )

    assert len(httpserver.log) == 1
    req = httpserver.log[0][0]
    body = req.get_data()

    envelope = Envelope.deserialize(body)

    assert_meta(envelope, "🤮🚀")
    assert_breadcrumb(envelope)
    assert_stacktrace(envelope)

    assert_event(envelope)