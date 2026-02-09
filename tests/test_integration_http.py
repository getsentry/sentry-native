import itertools
import json
import os
import time

import pytest
from flaky import flaky

from . import (
    make_dsn,
    run,
    Envelope,
    split_log_request_cond,
    is_feedback_envelope,
    SENTRY_VERSION,
)
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
    assert_user_report,
    assert_minidump,
    assert_breakpad_crash,
    assert_gzip_content_encoding,
    assert_gzip_file_header,
    assert_attachment_view_hierarchy,
)
from .conditions import has_http, has_breakpad, has_files, is_kcov

pytestmark = pytest.mark.skipif(not has_http, reason="tests need http")

# fmt: off
auth_header = (
    f"Sentry sentry_key=uiaeosnrtdy, sentry_version=7, sentry_client=sentry.native/{SENTRY_VERSION}"
)
# fmt: on


@pytest.mark.parametrize(
    "build_args",
    [
        ({"SENTRY_TRANSPORT_COMPRESSION": "Off"}),
        ({"SENTRY_TRANSPORT_COMPRESSION": "On"}),
    ],
)
def test_capture_http(cmake, httpserver, build_args):
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
        ["log", "release-env", "capture-event", "add-stacktrace"],
        env=env,
    )

    assert len(httpserver.log) == 1
    req = httpserver.log[0][0]
    body = req.get_data()

    if build_args.get("SENTRY_TRANSPORT_COMPRESSION") == "On":
        assert_gzip_content_encoding(req)
        assert_gzip_file_header(body)

    envelope = Envelope.deserialize(body)

    assert_meta(envelope, "🤮🚀")
    assert_breadcrumb(envelope)
    assert_stacktrace(envelope)

    assert_event(envelope)


def test_session_http(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    # start once without a release, but with a session
    run(
        tmp_path,
        "sentry_example",
        ["log", "release-env", "start-session"],
        env=env,
    )
    run(
        tmp_path,
        "sentry_example",
        ["log", "start-session"],
        env=env,
    )

    assert len(httpserver.log) == 1
    output = httpserver.log[0][0].get_data()
    envelope = Envelope.deserialize(output)

    assert_session(envelope, {"init": True, "status": "exited", "errors": 0})


def test_capture_and_session_http(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "start-session", "capture-event"],
        env=env,
    )

    assert len(httpserver.log) == 2
    output = httpserver.log[0][0].get_data()
    envelope = Envelope.deserialize(output)

    assert_event(envelope)
    assert_session(envelope, {"init": True, "status": "ok", "errors": 0})

    output = httpserver.log[1][0].get_data()
    envelope = Envelope.deserialize(output)
    assert_session(envelope, {"status": "exited", "errors": 0})


def test_user_feedback_http(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "capture-user-feedback"],
        env=env,
    )

    assert len(httpserver.log) == 1
    output = httpserver.log[0][0].get_data()
    envelope = Envelope.deserialize(output)

    assert_user_feedback(envelope)


def test_user_feedback_with_attachments_http(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "capture-user-feedback-with-attachment"],
        env=env,
    )

    assert len(httpserver.log) == 1
    output = httpserver.log[0][0].get_data()
    envelope = Envelope.deserialize(output)

    # Verify the feedback is present
    assert_user_feedback(envelope)

    # Verify attachments are present
    attachment_count = 0
    for item in envelope:
        if item.headers.get("type") == "attachment":
            attachment_count += 1

    # Should have 2 attachments (one file, one bytes)
    assert attachment_count == 2


def test_user_report_http(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "capture-user-report"],
        env=env,
    )

    assert len(httpserver.log) == 2
    output = httpserver.log[0][0].get_data()
    envelope = Envelope.deserialize(output)

    assert_event(envelope, "Hello user feedback!")

    output = httpserver.log[1][0].get_data()
    envelope = Envelope.deserialize(output)

    assert_user_report(envelope)


@pytest.mark.skipif(is_kcov, reason="kcov exits with 0 even when the process crashes")
@pytest.mark.parametrize(
    "build_args",
    [
        ({"SENTRY_BACKEND": "inproc"}),
        pytest.param(
            {"SENTRY_BACKEND": "breakpad"},
            marks=pytest.mark.skipif(
                not has_breakpad, reason="test needs breakpad backend"
            ),
        ),
    ],
)
def test_external_crash_reporter_http(cmake, httpserver, build_args):
    tmp_path = cmake(["sentry_example", "sentry_crash_reporter"], build_args)

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    with httpserver.wait(timeout=10) as waiting:
        env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
        run(
            tmp_path,
            "sentry_example",
            ["log", "crash-reporter", "crash"],
            expect_failure=True,
            env=env,
        )

        # the session crash heuristic on Mac uses timestamps, so make sure we have
        # a small delay here
        time.sleep(1)

        run(
            tmp_path,
            "sentry_example",
            ["log", "no-setup"],
            env=env,
        )
    assert waiting.result

    assert len(httpserver.log) == 2
    feedback_request, crash_request = split_log_request_cond(
        httpserver.log, is_feedback_envelope
    )
    feedback = feedback_request.get_data()
    crash = crash_request.get_data()

    envelope = Envelope.deserialize(crash)
    assert_meta(envelope, integration=build_args.get("SENTRY_BACKEND", ""))

    envelope = Envelope.deserialize(feedback)
    assert_user_feedback(envelope)


def test_exception_and_session_http(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "start-session", "capture-exception", "add-stacktrace"],
        env=env,
    )

    assert len(httpserver.log) == 2
    output = httpserver.log[0][0].get_data()
    envelope = Envelope.deserialize(output)

    assert_exception(envelope)
    assert_stacktrace(envelope, inside_exception=True)
    assert_session(envelope, {"init": True, "status": "ok", "errors": 1})

    output = httpserver.log[1][0].get_data()
    envelope = Envelope.deserialize(output)
    assert_session(envelope, {"status": "exited", "errors": 1})


@pytest.mark.skipif(not has_files, reason="test needs a local filesystem")
def test_abnormal_session(cmake, httpserver):
    tmp_path = cmake(
        ["sentry_example"],
        {"SENTRY_BACKEND": "none"},
    )

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    # create a bogus session file
    session = json.dumps(
        {
            "sid": "00000000-0000-0000-0000-000000000000",
            "did": "42",
            "status": "started",
            "errors": 0,
            "started": "2020-06-02T10:04:53.680Z",
            "duration": 10,
            "attrs": {"release": "test-example-release", "environment": "development"},
        }
    )
    db_dir = tmp_path.joinpath(".sentry-native")
    db_dir.mkdir(exist_ok=True)
    # 101 exceeds the max session items
    for i in range(101):
        run_dir = db_dir.joinpath(f"foo-{i}.run")
        run_dir.mkdir()
        with open(run_dir.joinpath("session.json"), "w") as session_file:
            session_file.write(session)

    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=env,
    )

    assert len(httpserver.log) == 2
    envelope1 = Envelope.deserialize(httpserver.log[0][0].get_data())
    envelope2 = Envelope.deserialize(httpserver.log[1][0].get_data())

    session_count = 0
    for item in itertools.chain(envelope1, envelope2):
        if item.headers.get("type") == "session":
            session_count += 1
    assert session_count == 101

    assert_session(envelope1, {"status": "abnormal", "errors": 0, "duration": 10})


@pytest.mark.parametrize(
    "build_args",
    [
        ({"SENTRY_TRANSPORT_COMPRESSION": "Off"}),
        ({"SENTRY_TRANSPORT_COMPRESSION": "On"}),
    ],
)
def test_inproc_crash_http(cmake, httpserver, build_args):
    build_args.update({"SENTRY_BACKEND": "inproc"})
    tmp_path = cmake(["sentry_example"], build_args)

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "start-session", "attachment", "attach-view-hierarchy", "crash"],
        expect_failure=True,
        env=env,
    )

    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=env,
    )

    assert len(httpserver.log) == 1
    req = httpserver.log[0][0]
    body = req.get_data()

    if build_args.get("SENTRY_TRANSPORT_COMPRESSION") == "On":
        assert_gzip_content_encoding(req)
        assert_gzip_file_header(body)

    envelope = Envelope.deserialize(body)

    assert_session(envelope, {"init": True, "status": "crashed", "errors": 1})

    assert_meta(envelope, integration="inproc")
    assert_breadcrumb(envelope)
    assert_attachment(envelope)
    assert_attachment_view_hierarchy(envelope)

    assert_inproc_crash(envelope)


def test_inproc_reinstall(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "inproc"})

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "reinstall", "crash"],
        expect_failure=True,
        env=env,
    )

    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=env,
    )

    assert len(httpserver.log) == 1


def test_inproc_dump_inflight(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "inproc"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "capture-multiple", "crash"],
        expect_failure=True,
        env=env,
    )
    run(tmp_path, "sentry_example", ["log", "no-setup"], env=env)

    # we trigger 10 normal events, and 1 crash
    assert len(httpserver.log) >= 11


@pytest.mark.skipif(not has_breakpad, reason="test needs breakpad backend")
@pytest.mark.parametrize(
    "build_args",
    [
        ({"SENTRY_TRANSPORT_COMPRESSION": "Off"}),
        ({"SENTRY_TRANSPORT_COMPRESSION": "On"}),
    ],
)
def test_breakpad_crash_http(cmake, httpserver, build_args):
    build_args.update({"SENTRY_BACKEND": "breakpad"})
    tmp_path = cmake(["sentry_example"], build_args)

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "start-session", "attachment", "attach-view-hierarchy", "crash"],
        expect_failure=True,
        env=env,
    )

    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=env,
    )

    assert len(httpserver.log) == 1
    req = httpserver.log[0][0]
    body = req.get_data()

    if build_args.get("SENTRY_TRANSPORT_COMPRESSION") == "On":
        assert_gzip_content_encoding(req)
        assert_gzip_file_header(body)

    envelope = Envelope.deserialize(body)

    assert_session(envelope, {"init": True, "status": "crashed", "errors": 1})

    assert_meta(envelope, integration="breakpad")
    assert_breadcrumb(envelope)
    assert_attachment(envelope)
    assert_attachment_view_hierarchy(envelope)

    assert_breakpad_crash(envelope)
    assert_minidump(envelope)


@pytest.mark.skipif(not has_breakpad, reason="test needs breakpad backend")
def test_breakpad_reinstall(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "breakpad"})

    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "reinstall", "crash"],
        expect_failure=True,
        env=env,
    )

    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=env,
    )

    assert len(httpserver.log) == 1


@pytest.mark.skipif(not has_breakpad, reason="test needs breakpad backend")
@flaky(max_runs=3)
def test_breakpad_dump_inflight(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "breakpad"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "capture-multiple", "crash"],
        expect_failure=True,
        env=env,
        timeout=300,
    )

    run(tmp_path, "sentry_example", ["log", "no-setup"], env=env)

    # we trigger 10 normal events, and 1 crash
    assert len(httpserver.log) >= 11


@flaky(max_runs=3)
def test_shutdown_timeout(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    # the timings here are:
    # * the process waits 2s for the background thread to shut down, which fails
    # * it then dumps everything and waits another 1s before terminating the process
    # * the python runner waits for 2.4s in total to close the request, which
    #   will cleanly terminate the background worker.
    # the assumption here is that 2s < 2.4s < 2s+1s. but since those timers
    # run in different processes, this has the potential of being flaky

    def delayed(req):
        time.sleep(2.4)
        return "{}"

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_handler(delayed)
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    # Using `sleep-after-shutdown` here means that the background worker will
    # deref/free itself, so we will not leak in that case!
    run(
        tmp_path,
        "sentry_example",
        ["log", "capture-multiple", "sleep-after-shutdown"],
        env=env,
    )

    httpserver.clear_all_handlers()
    httpserver.clear_log()

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(tmp_path, "sentry_example", ["log", "no-setup"], env=env)

    assert len(httpserver.log) == 10


def test_capture_minidump(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "attachment", "attach-view-hierarchy", "capture-minidump"],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) == 1

    req = httpserver.log[0][0]
    body = req.get_data()

    envelope = Envelope.deserialize(body)

    assert_breadcrumb(envelope)
    assert_attachment(envelope)
    assert_attachment_view_hierarchy(envelope)

    assert_minidump(envelope)


def test_capture_with_scope(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "attach-to-scope", "capture-with-scope"],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) == 1

    req = httpserver.log[0][0]
    body = req.get_data()

    envelope = Envelope.deserialize(body)

    assert_breadcrumb(envelope, "scoped crumb")
    assert_attachment(envelope)
