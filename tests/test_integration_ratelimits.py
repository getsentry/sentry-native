import pytest
import os
from . import cmake, make_dsn, run
from .conditions import has_http

if not has_http:
    pytest.skip("tests need http", allow_module_level=True)

def test_retry_after(tmp_path, httpserver):
    # we want to have the default transport
    cmake(tmp_path, ["sentry_example"], {"SENTRY_BACKEND": "none"})

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    # should respect retry-after also with `200` requests
    httpserver.expect_oneshot_request(
        "/api/123456/store/").respond_with_data('OK', 200, {"retry-after": "60"})
    run(tmp_path, "sentry_example", ["capture-multiple"],
        check=True, env=env)
    assert len(httpserver.log) == 1

    httpserver.expect_oneshot_request(
        "/api/123456/store/").respond_with_data('OK', 429, {"retry-after": "60"})
    run(tmp_path, "sentry_example", ["capture-multiple"],
        check=True, env=env)
    assert len(httpserver.log) == 2

def test_rate_limits(tmp_path, httpserver):
    cmake(tmp_path, ["sentry_example"], {"SENTRY_BACKEND": "none"})

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    headers = {"X-Sentry-Rate-Limits": "60::organization"}

    httpserver.expect_oneshot_request(
        "/api/123456/store/").respond_with_data('OK', 200, headers)
    run(tmp_path, "sentry_example", ["capture-multiple"],
        check=True, env=env)
    assert len(httpserver.log) == 1

    httpserver.expect_oneshot_request(
        "/api/123456/store/").respond_with_data('OK', 429, headers)
    run(tmp_path, "sentry_example", ["capture-multiple"],
        check=True, env=env)
    assert len(httpserver.log) == 2
