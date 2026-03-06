import os
import pytest

from . import make_dsn, run, Envelope
from .assertions import (
    assert_client_report,
    assert_no_client_report,
    assert_event,
    assert_session,
)
from .conditions import has_http

pytestmark = pytest.mark.skipif(not has_http, reason="tests need http")


def test_client_report_none(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "capture-event"],
        env=env,
    )

    assert len(httpserver.log) == 1
    envelope = Envelope.deserialize(httpserver.log[0][0].get_data())

    assert_no_client_report(envelope)
    assert_event(envelope)


def test_client_report_before_send(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request("/api/123456/envelope/").respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    # The event is discarded by before_send. The session envelope sent at
    # shutdown acts as a carrier for the client report.
    run(
        tmp_path,
        "sentry_example",
        ["log", "start-session", "discarding-before-send", "capture-event"],
        env=env,
    )

    assert len(httpserver.log) == 1
    envelope = Envelope.deserialize(httpserver.log[0][0].get_data())

    assert_session(envelope)
    assert_client_report(
        envelope,
        [{"reason": "before_send", "category": "error", "quantity": 1}],
    )


def test_client_report_ratelimit(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    # The first event gets through but triggers a rate limit for the "error"
    # category only. The error items of the remaining 9 events are filtered out
    # during serialization, but their session updates still pass through (not
    # rate-limited). Each subsequent envelope carries client report discards
    # incrementally, so we aggregate across all envelopes.
    request_count = [0]

    def ratelimit_first(request):
        from werkzeug import Response

        request_count[0] += 1
        if request_count[0] == 1:
            return Response(
                "OK", 200, {"X-Sentry-Rate-Limits": "60:error:organization"}
            )
        return Response("OK", 200)

    httpserver.expect_request("/api/123456/envelope/").respond_with_handler(
        ratelimit_first
    )
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "start-session", "capture-multiple"],
        env=env,
    )

    # First envelope: event + session (before rate limit).
    # No event items in subsequent envelopes (rate-limited), but session
    # updates still go through.
    assert len(httpserver.log) >= 2

    total_discards = {}
    for req, _resp in httpserver.log:
        envelope = Envelope.deserialize(req.get_data())
        for item in envelope:
            if item.headers.get("type") != "client_report" or not item.payload.json:
                continue
            for entry in item.payload.json.get("discarded_events", []):
                key = (entry["reason"], entry["category"])
                total_discards[key] = total_discards.get(key, 0) + entry["quantity"]

    assert total_discards == {("ratelimit_backoff", "error"): 9}
