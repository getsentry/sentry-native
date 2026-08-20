import os

import pytest

from . import Envelope, SENTRY_VERSION, make_dsn, run
from .conditions import has_http

pytestmark = pytest.mark.skipif(not has_http, reason="tests need http transport")


def test_platform_integration(cmake, httpserver):
    cwd = cmake(
        ["sentry_test_platform"],
        {
            "SENTRY_BACKEND": "none",
            "SENTRY_BUILD_SHARED_LIBS": "OFF",
            "SENTRY_INTEGRATION_PLATFORM": "ON",
        },
    )

    httpserver.expect_oneshot_request("/api/123456/envelope/").respond_with_data("OK")

    run(
        cwd,
        "sentry_test_platform",
        [],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) == 1
    envelope = Envelope.deserialize(httpserver.log[0][0].get_data())

    (item,) = envelope.items
    assert item.headers["type"] == "event"
    event = item.payload.json

    # SDK
    assert event["platform"] == "native"
    assert event["environment"] == "production"
    assert event["event_id"]
    assert event["contexts"]["os"]["name"]
    assert len(event["contexts"]["trace"]["trace_id"]) == 32
    assert len(event["contexts"]["trace"]["span_id"]) == 16
    assert event["sdk"]["version"] == SENTRY_VERSION
    assert event["sdk"]["packages"] == [
        {
            "name": "github:getsentry/sentry-native",
            "version": SENTRY_VERSION,
        }
    ]

    # platform integration
    assert event["contexts"]["device"] == {
        "name": "Test",
        "model": "test-model",
        "arch": "test-arch",
    }
    assert event["sdk"]["name"] == "sentry.native.test"
    assert event["sdk"]["integrations"].count("test") == 1

    # app
    assert event["tags"] == {"my-tag": "my-value"}
    assert event["user"] == {"id": "123", "username": "my-user"}
