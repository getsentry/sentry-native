import os
import shutil

import pytest

from . import make_dsn, run, Envelope
from .assertions import (
    assert_attachment,
    assert_breadcrumb,
    assert_minidump,
)
from .conditions import has_http

pytestmark = pytest.mark.skipif(not has_http, reason="tests need http")

# fmt: off
auth_header = (
    "Sentry sentry_key=uiaeosnrtdy, sentry_version=7, sentry_client=sentry.native/0.7.11"
)
# fmt: on


def test_capture_minidump(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    # make sure we are isolated from previous runs
    shutil.rmtree(tmp_path / ".sentry-native", ignore_errors=True)

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "attachment", "capture-minidump"],
        check=True,
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) == 1

    req = httpserver.log[0][0]
    body = req.get_data()

    envelope = Envelope.deserialize(body)

    assert_breadcrumb(envelope)
    assert_attachment(envelope)

    assert_minidump(envelope)
