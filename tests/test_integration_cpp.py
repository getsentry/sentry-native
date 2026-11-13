import email
import gzip
import os
import subprocess
import sys

import pytest

from . import Envelope, check_output, make_dsn, run
from .assertions import _load_crashpad_attachments
from .conditions import (
    has_breakpad,
    has_crashpad,
    has_files,
    has_native,
    is_qemu,
    is_wine,
)

pytestmark = pytest.mark.skipif(
    not has_files,
    reason="needs local crash storage",
)


@pytest.mark.skipif(is_qemu, reason="unreliable under qemu-user")
@pytest.mark.skipif(is_wine, reason="unsupported under Wine")
@pytest.mark.parametrize(
    "type_name,message",
    [
        pytest.param("runtime_error", "something went wrong", id="runtime_error"),
        pytest.param("out_of_range", None, id="out_of_range"),
        pytest.param("invalid_argument", None, id="invalid_argument"),
        pytest.param("custom_exception", "custom exception", id="custom_exception"),
        pytest.param("range_error", "x" * 1023, id="range_error"),
    ],
)
@pytest.mark.parametrize(
    "backend",
    [
        "inproc",
        pytest.param(
            "breakpad",
            marks=pytest.mark.skipif(not has_breakpad, reason="needs breakpad"),
        ),
        pytest.param(
            "crashpad",
            marks=[
                pytest.mark.skipif(not has_crashpad, reason="needs crashpad"),
                pytest.mark.skipif(
                    sys.platform == "darwin",
                    reason="crashpad has no first-chance handler on macOS",
                ),
            ],
        ),
        pytest.param(
            "native",
            marks=pytest.mark.skipif(not has_native, reason="needs native"),
        ),
    ],
)
def test_uncaught_cpp_exception(cmake, httpserver, backend, type_name, message):
    build_options = {
        "SENTRY_BACKEND": backend,
        "SENTRY_INTEGRATION_CPP": "ON",
    }
    if backend not in ("native", "crashpad"):
        build_options["SENTRY_TRANSPORT"] = "none"

    tmp_path = cmake(
        ["sentry_test_cpp", "sentry_test_integration"],
        build_options,
    )

    if backend in ("native", "crashpad"):
        path = (
            "/api/123456/envelope/" if backend == "native" else "/api/123456/minidump/"
        )
        httpserver.expect_oneshot_request(path).respond_with_data("OK")
        with httpserver.wait(timeout=10) as waiting:
            child = run(
                tmp_path,
                "sentry_test_cpp",
                [type_name],
                expect_failure=True,
                env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
                stdout=subprocess.PIPE,
            )
        assert waiting.result
    else:
        child = run(
            tmp_path,
            "sentry_test_cpp",
            [type_name],
            expect_failure=True,
            stdout=subprocess.PIPE,
        )

    output = child.stdout.decode().splitlines()
    # MSVC/ClangCL: SEH vs. MinGW: std::terminate
    if sys.platform != "win32" or os.environ.get("TEST_MINGW"):
        assert output.pop(0) == "terminate_handler"
    observed_type, observed_value = output
    assert type_name in observed_type
    if sys.platform != "win32" or os.environ.get("TEST_MINGW"):
        assert observed_type == type_name or "::" in observed_type
    if message is None:
        assert observed_value
    else:
        assert observed_value == message

    if backend == "native":
        request = httpserver.log[0][0]
        assert request.path == "/api/123456/envelope/"
        event = Envelope.deserialize(request.get_data()).get_event()
    elif backend == "crashpad":
        request = httpserver.log[0][0]
        assert request.path.startswith("/api/123456/minidump/")
        multipart = gzip.decompress(request.get_data())
        multipart_message = email.message_from_bytes(
            bytes(str(request.headers), encoding="utf8") + multipart
        )
        attachments = _load_crashpad_attachments(multipart_message)
        assert attachments.minidump.startswith(b"MDMP")
        event = attachments.event
    else:
        output = check_output(
            tmp_path, "sentry_test_integration", ["stdout", "no-setup"]
        )
        event = Envelope.deserialize(output).get_event()

    exception = event["exception"]["values"][0]
    assert exception["type"] == observed_type
    assert exception["value"] == observed_value
    assert exception["mechanism"]["type"] == "cpp_exception"


@pytest.mark.skipif(is_qemu, reason="unreliable under qemu-user")
@pytest.mark.parametrize("mode", ["int", "string"])
def test_nonstd_exception(cmake, mode):
    tmp_path = cmake(
        ["sentry_test_cpp", "sentry_test_integration"],
        {
            "SENTRY_BACKEND": "inproc",
            "SENTRY_TRANSPORT": "none",
            "SENTRY_INTEGRATION_CPP": "ON",
        },
    )

    run(tmp_path, "sentry_test_cpp", [mode], expect_failure=True)
    output = check_output(tmp_path, "sentry_test_integration", ["stdout", "no-setup"])
    event = Envelope.deserialize(output).get_event()
    assert event["exception"]["values"][0]["mechanism"]["type"] == "signalhandler"


@pytest.mark.skipif(not has_native, reason="needs native")
@pytest.mark.skipif(is_qemu or is_wine, reason="needs native execution")
@pytest.mark.skipif(bool(os.environ.get("TEST_MINGW")), reason="no throw hook")
@pytest.mark.parametrize("mode", ["throw_stack", "rethrow_stack"])
def test_throw_stack(cmake, httpserver, mode):
    tmp_path = cmake(
        ["sentry_test_cpp"],
        {"SENTRY_BACKEND": "native", "SENTRY_INTEGRATION_CPP": "ON"},
    )
    httpserver.expect_oneshot_request("/api/123456/envelope/").respond_with_data("OK")
    with httpserver.wait(timeout=10) as waiting:
        child = run(
            tmp_path,
            "sentry_test_cpp",
            [mode],
            expect_failure=True,
            env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
            stdout=subprocess.PIPE,
        )
    assert waiting.result
    caller = next(
        int(line.split(":", 1)[1], 16)
        for line in child.stdout.decode().splitlines()
        if line.startswith("THROW_CALLER:")
    )
    event = Envelope.deserialize(httpserver.log[0][0].get_data()).get_event()
    exception = event["exception"]["values"][0]
    assert exception["value"] == "something went wrong"
    assert exception["mechanism"]["data"]["throw_stacktrace"] is True
    stacktrace = exception["stacktrace"]
    assert caller in {
        int(frame["instruction_addr"], 16) for frame in stacktrace["frames"]
    }
    assert "registers" not in stacktrace


@pytest.mark.skipif(not has_native, reason="needs native")
@pytest.mark.skipif(is_qemu or is_wine, reason="needs native execution")
def test_reused_throw_stack(cmake, httpserver):
    tmp_path = cmake(
        ["sentry_test_cpp"],
        {"SENTRY_BACKEND": "native", "SENTRY_INTEGRATION_CPP": "ON"},
    )
    httpserver.expect_oneshot_request("/api/123456/envelope/").respond_with_data("OK")
    with httpserver.wait(timeout=10) as waiting:
        child = run(
            tmp_path,
            "sentry_test_cpp",
            ["reused_stack"],
            expect_failure=True,
            env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
            stdout=subprocess.PIPE,
        )
    assert waiting.result
    caller = next(
        int(line.split(":", 1)[1], 16)
        for line in child.stdout.decode().splitlines()
        if line.startswith("THROW_CALLER:")
    )
    event = Envelope.deserialize(httpserver.log[0][0].get_data()).get_event()
    exception = event["exception"]["values"][0]
    assert exception["value"] == "fresh"
    assert caller not in {
        int(frame["instruction_addr"], 16)
        for frame in exception["stacktrace"]["frames"]
    }
