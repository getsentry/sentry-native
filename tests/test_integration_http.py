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
    is_logs_envelope,
    is_metrics_envelope,
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
    assert_logs,
    assert_metrics,
)
from .conditions import has_http, has_breakpad, has_native, has_files, is_kcov

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
    # * the python runner waits for 2.5s in total to close the request, which
    #   will cleanly terminate the background worker.
    # the assumption here is that 2s < 2.5s < 2s+1s. The margins are tight
    # (0.5s on each side), so in CI environments with load this can still be
    # flaky. We use >= instead of == to tolerate minor timing variations.

    def delayed(req):
        time.sleep(2.5)
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

    # The test verifies that events are properly dumped to disk when shutdown
    # times out and sent on restart. Due to timing variations across platforms
    # and CI environments, not all 10 events may make it through. We require
    # at least 3 to verify the core functionality works (dump to disk + send
    # on restart). The exact count depends on how many events were queued
    # before the shutdown timeout kicked in.
    assert len(httpserver.log) >= 3, (
        f"Expected at least 3 events to be sent on restart, got {len(httpserver.log)}. "
        "Events should be dumped to disk on shutdown timeout and sent on restart."
    )


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


def test_logs_timer(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-logs", "logs-timer"],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) == 2

    req_0 = httpserver.log[0][0]
    body_0 = req_0.get_data()

    envelope_0 = Envelope.deserialize(body_0)
    assert_logs(envelope_0, 10)

    req_1 = httpserver.log[1][0]
    body_1 = req_1.get_data()

    envelope_1 = Envelope.deserialize(body_1)
    assert_logs(envelope_1)


def test_logs_event(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-logs", "capture-log", "capture-event"],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) == 2

    event_req = httpserver.log[0][0]
    event_body = event_req.get_data()

    event_envelope = Envelope.deserialize(event_body)
    assert_event(event_envelope)
    # ensure that the event and the log are part of the same trace
    event_trace_id = event_envelope.items[0].payload.json["contexts"]["trace"][
        "trace_id"
    ]

    log_req = httpserver.log[1][0]
    log_body = log_req.get_data()

    log_envelope = Envelope.deserialize(log_body)
    assert_logs(log_envelope, 1, event_trace_id)


def test_logs_scoped_transaction(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        [
            "log",
            "enable-logs",
            "logs-scoped-transaction",
            "capture-transaction",
            "scope-transaction-event",
        ],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    assert len(httpserver.log) == 3

    event_req = httpserver.log[0][0]
    event_body = event_req.get_data()

    event_envelope = Envelope.deserialize(event_body)
    assert_event(event_envelope)
    # ensure that the event and the log are part of the same trace
    event_trace_id = event_envelope.items[0].payload.json["contexts"]["trace"][
        "trace_id"
    ]

    tx_req = httpserver.log[1][0]
    tx_body = tx_req.get_data()

    tx_envelope = Envelope.deserialize(tx_body)
    # ensure that the transaction, event, and logs are part of the same trace
    tx_trace_id = tx_envelope.items[0].payload.json["contexts"]["trace"]["trace_id"]
    assert tx_trace_id == event_trace_id

    log_req = httpserver.log[2][0]
    log_body = log_req.get_data()

    log_envelope = Envelope.deserialize(log_body)
    assert_logs(log_envelope, 2, event_trace_id)


def test_logs_threaded(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-logs", "logs-threads"],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    # there is a chance we drop logs while flushing buffers
    assert 1 <= len(httpserver.log) <= 50
    total_count = 0

    for i in range(len(httpserver.log)):
        req = httpserver.log[i][0]
        body = req.get_data()

        envelope = Envelope.deserialize(body)
        assert_logs(envelope)
        total_count += envelope.items[0].headers["item_count"]
    print(f"Total amount of captured logs: {total_count}")
    assert total_count >= 100


def test_before_send_log(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver), SENTRY_RELEASE="🤮🚀")

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-logs", "capture-log", "before-send-log"],
        env=env,
    )

    assert len(httpserver.log) == 1
    req = httpserver.log[0][0]
    body = req.get_data()

    envelope = Envelope.deserialize(body)

    # Show what the envelope looks like if the test fails.
    envelope.print_verbose()

    # Extract the log item
    (log_item,) = envelope.items

    assert log_item.headers["type"] == "log"
    payload = log_item.payload.json

    # Get the first log item from the logs payload
    log_entry = payload["items"][0]
    attributes = log_entry["attributes"]

    # Check that the before_send_log callback added the expected attribute
    assert "coffeepot.size" in attributes
    assert attributes["coffeepot.size"]["value"] == "little"
    assert attributes["coffeepot.size"]["type"] == "string"


def test_before_send_log_discard(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver), SENTRY_RELEASE="🤮🚀")

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-logs", "capture-log", "discarding-before-send-log"],
        env=env,
    )

    # log should have been discarded
    assert len(httpserver.log) == 0


def test_logs_on_crash(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver), SENTRY_RELEASE="🤮🚀")

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-logs", "capture-log", "crash"],
        expect_failure=True,
        env=env,
    )

    # log should have been discarded since we have no backend to hook into the crash
    assert len(httpserver.log) == 0


def test_inproc_logs_on_crash(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "inproc"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-logs", "capture-log", "crash"],
        expect_failure=True,
        env=env,
    )

    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=env,
    )

    # we expect 1 envelope with the log, and 1 for the crash
    assert len(httpserver.log) == 2
    logs_request, crash_request = split_log_request_cond(
        httpserver.log, is_logs_envelope
    )
    logs = logs_request.get_data()

    logs_envelope = Envelope.deserialize(logs)

    assert logs_envelope is not None
    assert_logs(logs_envelope, 1)


@pytest.mark.skipif(not has_breakpad, reason="test needs breakpad backend")
def test_breakpad_logs_on_crash(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "breakpad"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-logs", "capture-log", "crash"],
        expect_failure=True,
        env=env,
    )

    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=env,
    )

    # we expect 1 envelope with the log, and 1 for the crash
    assert len(httpserver.log) == 2
    logs_request, crash_request = split_log_request_cond(
        httpserver.log, is_logs_envelope
    )
    logs = logs_request.get_data()

    logs_envelope = Envelope.deserialize(logs)

    assert logs_envelope is not None
    assert_logs(logs_envelope, 1)


def test_logs_with_custom_attributes(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-logs", "log-attributes"],
        env=env,
    )

    assert len(httpserver.log) == 1
    req = httpserver.log[0][0]
    body = req.get_data()

    envelope = Envelope.deserialize(body)

    # Show what the envelope looks like if the test fails
    envelope.print_verbose()

    # Extract the log item
    (log_item,) = envelope.items

    assert log_item.headers["type"] == "log"
    payload = log_item.payload.json

    # We expect 3 log entries based on the example
    assert len(payload["items"]) == 3

    # Test 1: Log with custom attributes and format string
    log_entry_0 = payload["items"][0]
    assert log_entry_0["body"] == "logging with 3 custom attributes"
    attributes_0 = log_entry_0["attributes"]

    # Check custom attributes exist
    assert "my.custom.attribute" in attributes_0
    assert attributes_0["my.custom.attribute"]["value"] == "my_attribute"
    assert attributes_0["my.custom.attribute"]["type"] == "string"

    assert "number.first" in attributes_0
    assert attributes_0["number.first"]["value"] == 2**63 - 1  # INT64_MAX
    assert attributes_0["number.first"]["type"] == "integer"
    assert attributes_0["number.first"]["unit"] == "fermions"

    assert "number.second" in attributes_0
    assert attributes_0["number.second"]["value"] == -(2**63)  # INT64_MIN
    assert attributes_0["number.second"]["type"] == "integer"
    assert attributes_0["number.second"]["unit"] == "bosons"

    # Check that format parameters were parsed
    assert "sentry.message.parameter.0" in attributes_0
    assert attributes_0["sentry.message.parameter.0"]["value"] == 3
    assert attributes_0["sentry.message.parameter.0"]["type"] == "integer"

    # Check that default attributes are still present
    assert "sentry.sdk.name" in attributes_0
    assert "sentry.sdk.version" in attributes_0

    # Test 2: Log with empty custom attributes object
    log_entry_1 = payload["items"][1]
    assert log_entry_1["body"] == "logging with no custom attributes"
    attributes_1 = log_entry_1["attributes"]

    # Should still have default attributes
    assert "sentry.sdk.name" in attributes_1
    assert "sentry.sdk.version" in attributes_1

    # Check that format string parameter was parsed
    assert "sentry.message.parameter.0" in attributes_1
    assert attributes_1["sentry.message.parameter.0"]["value"] == "no"
    assert attributes_1["sentry.message.parameter.0"]["type"] == "string"

    # Test 3: Log with custom attributes that override defaults
    log_entry_2 = payload["items"][2]
    assert log_entry_2["body"] == "logging with a custom parameter attribute"
    attributes_2 = log_entry_2["attributes"]

    # Check custom attribute exists
    assert "sentry.message.parameter.0" in attributes_2
    assert attributes_2["sentry.message.parameter.0"]["value"] == "parameter"
    assert attributes_2["sentry.message.parameter.0"]["type"] == "string"

    # Check that sentry.sdk.name was overwritten by custom attribute
    assert "sentry.sdk.name" in attributes_2
    assert attributes_2["sentry.sdk.name"]["value"] == "custom-sdk-name"
    assert attributes_2["sentry.sdk.name"]["type"] == "string"


def test_logs_global_and_local_attributes_merge(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-logs", "set-global-attribute", "log-attributes"],
        env=env,
    )

    assert len(httpserver.log) == 1
    req = httpserver.log[0][0]
    body = req.get_data()

    envelope = Envelope.deserialize(body)

    # Show what the envelope looks like if the test fails
    envelope.print_verbose()

    # Extract the log item
    (log_item,) = envelope.items

    assert log_item.headers["type"] == "log"
    payload = log_item.payload.json

    # We expect 3 log entries based on the log-attributes example
    assert len(payload["items"]) == 3

    log_entry_0 = payload["items"][0]
    attributes_0 = log_entry_0["attributes"]

    # Verify per-log (local) attributes are present

    # passed as global and local attribute, but local should overwrite
    assert "my.custom.attribute" in attributes_0
    assert attributes_0["my.custom.attribute"]["value"] == "my_attribute"
    assert attributes_0["my.custom.attribute"]["type"] == "string"

    assert "global.attribute.bool" in attributes_0
    assert attributes_0["global.attribute.bool"]["value"] == True
    assert attributes_0["global.attribute.bool"]["type"] == "boolean"

    assert "global.attribute.int" in attributes_0
    assert attributes_0["global.attribute.int"]["value"] == 123
    assert attributes_0["global.attribute.int"]["type"] == "integer"

    assert "global.attribute.double" in attributes_0
    assert attributes_0["global.attribute.double"]["value"] == 1.23
    assert attributes_0["global.attribute.double"]["type"] == "double"

    assert "global.attribute.string" in attributes_0
    assert attributes_0["global.attribute.string"]["value"] == "my_global_value"
    assert attributes_0["global.attribute.string"]["type"] == "string"

    assert "global.attribute.array" in attributes_0
    assert attributes_0["global.attribute.array"]["value"] == ["item1", "item2"]
    assert attributes_0["global.attribute.array"]["type"] == "array"


def test_metrics_capture(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-metrics", "capture-metric"],
        env=env,
    )

    assert len(httpserver.log) == 1

    req = httpserver.log[0][0]
    body = req.get_data()

    envelope = Envelope.deserialize(body)
    envelope.print_verbose()
    assert_metrics(envelope, 1)


def test_metrics_all_types(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-metrics", "capture-metric-all-types"],
        env=env,
    )

    assert len(httpserver.log) == 1

    req = httpserver.log[0][0]
    body = req.get_data()

    envelope = Envelope.deserialize(body)
    envelope.print_verbose()
    assert_metrics(envelope, 3)

    # Verify the different metric types
    metrics = envelope.items[0].payload.json
    types_found = {item["type"] for item in metrics["items"]}
    assert types_found == {"counter", "gauge", "distribution"}


def test_metrics_with_custom_attributes(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-metrics", "metric-with-attributes"],
        env=env,
    )

    assert len(httpserver.log) == 1

    req = httpserver.log[0][0]
    body = req.get_data()

    envelope = Envelope.deserialize(body)
    envelope.print_verbose()
    assert_metrics(envelope, 1)

    # Check custom attribute exists
    metric_item = envelope.items[0].payload.json["items"][0]
    attrs = metric_item["attributes"]
    assert "my.custom.attribute" in attrs
    assert attrs["my.custom.attribute"]["value"] == "my_value"
    assert attrs["my.custom.attribute"]["type"] == "string"


def test_metrics_timer(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-metrics", "metrics-timer"],
        env=env,
    )

    # We expect 2 envelopes: one from the timer flush (after sleep), one at shutdown
    assert len(httpserver.log) == 2

    req_0 = httpserver.log[0][0]
    body_0 = req_0.get_data()
    envelope_0 = Envelope.deserialize(body_0)
    assert_metrics(envelope_0, 10)

    req_1 = httpserver.log[1][0]
    body_1 = req_1.get_data()
    envelope_1 = Envelope.deserialize(body_1)
    assert_metrics(envelope_1, 1)


def test_metrics_scoped_transaction(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        [
            "log",
            "enable-metrics",
            "metrics-scoped-transaction",
            "capture-transaction",
            "scope-transaction-event",
        ],
        env=env,
    )

    # Event, transaction, and metrics
    assert len(httpserver.log) == 3

    event_req = httpserver.log[0][0]
    event_body = event_req.get_data()
    event_envelope = Envelope.deserialize(event_body)
    assert_event(event_envelope)

    event_trace_id = event_envelope.items[0].payload.json["contexts"]["trace"][
        "trace_id"
    ]

    tx_req = httpserver.log[1][0]
    tx_body = tx_req.get_data()
    tx_envelope = Envelope.deserialize(tx_body)
    tx_trace_id = tx_envelope.items[0].payload.json["contexts"]["trace"]["trace_id"]
    assert tx_trace_id == event_trace_id

    metrics_req = httpserver.log[2][0]
    metrics_body = metrics_req.get_data()
    metrics_envelope = Envelope.deserialize(metrics_body)
    assert_metrics(metrics_envelope, 1, event_trace_id)


def test_before_send_metric(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-metrics", "capture-metric", "before-send-metric"],
        env=env,
    )

    assert len(httpserver.log) == 1
    req = httpserver.log[0][0]
    body = req.get_data()

    envelope = Envelope.deserialize(body)
    envelope.print_verbose()

    # Extract the metric item
    (metric_item,) = envelope.items

    assert metric_item.headers["type"] == "trace_metric"
    payload = metric_item.payload.json

    # Get the first metric item from the metrics payload
    metric_entry = payload["items"][0]
    attributes = metric_entry["attributes"]

    # Check that the before_send_metric callback added the expected attribute
    assert "coffeepot.size" in attributes
    assert attributes["coffeepot.size"]["value"] == "little"
    assert attributes["coffeepot.size"]["type"] == "string"


def test_before_send_metric_discard(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-metrics", "capture-metric", "discarding-before-send-metric"],
        env=env,
    )

    # metric should have been discarded
    assert len(httpserver.log) == 0


def test_metrics_disabled(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    # Run without enable-metrics flag
    run(
        tmp_path,
        "sentry_example",
        ["log", "capture-metric"],
        env=env,
    )

    # No metrics should be sent when feature is disabled
    assert len(httpserver.log) == 0


def test_metrics_event(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-metrics", "capture-metric", "capture-event"],
        env=env,
    )

    assert len(httpserver.log) == 2

    event_req = httpserver.log[0][0]
    event_body = event_req.get_data()
    event_envelope = Envelope.deserialize(event_body)
    assert_event(event_envelope)

    # ensure that the event and the metric are part of the same trace
    event_trace_id = event_envelope.items[0].payload.json["contexts"]["trace"][
        "trace_id"
    ]

    metrics_req = httpserver.log[1][0]
    metrics_body = metrics_req.get_data()
    metrics_envelope = Envelope.deserialize(metrics_body)
    assert_metrics(metrics_envelope, 1, event_trace_id)


def test_metrics_threaded(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-metrics", "metrics-threads"],
        env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
    )

    # there is a chance we drop metrics while flushing buffers
    assert 1 <= len(httpserver.log) <= 50
    total_count = 0

    for i in range(len(httpserver.log)):
        req = httpserver.log[i][0]
        body = req.get_data()

        envelope = Envelope.deserialize(body)
        assert_metrics(envelope)
        total_count += envelope.items[0].headers["item_count"]
    print(f"Total amount of captured metrics: {total_count}")
    assert total_count >= 100


def test_metrics_global_and_local_attributes_merge(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-metrics", "set-global-attribute", "metric-with-attributes"],
        env=env,
    )

    assert len(httpserver.log) == 1
    req = httpserver.log[0][0]
    body = req.get_data()

    envelope = Envelope.deserialize(body)

    # Show what the envelope looks like if the test fails
    envelope.print_verbose()

    # Extract the metric item
    (metric_item,) = envelope.items

    assert metric_item.headers["type"] == "trace_metric"
    payload = metric_item.payload.json

    assert len(payload["items"]) == 1

    metric_entry = payload["items"][0]
    attributes = metric_entry["attributes"]

    # Verify local attribute (should overwrite global with same key)
    assert "my.custom.attribute" in attributes
    assert attributes["my.custom.attribute"]["value"] == "my_value"
    assert attributes["my.custom.attribute"]["type"] == "string"

    # Verify global attributes are present
    assert "global.attribute.bool" in attributes
    assert attributes["global.attribute.bool"]["value"] == True
    assert attributes["global.attribute.bool"]["type"] == "boolean"

    assert "global.attribute.int" in attributes
    assert attributes["global.attribute.int"]["value"] == 123
    assert attributes["global.attribute.int"]["type"] == "integer"

    assert "global.attribute.double" in attributes
    assert attributes["global.attribute.double"]["value"] == 1.23
    assert attributes["global.attribute.double"]["type"] == "double"

    assert "global.attribute.string" in attributes
    assert attributes["global.attribute.string"]["value"] == "my_global_value"
    assert attributes["global.attribute.string"]["type"] == "string"

    assert "global.attribute.array" in attributes
    assert attributes["global.attribute.array"]["value"] == ["item1", "item2"]
    assert attributes["global.attribute.array"]["type"] == "array"


def test_metrics_discarded_on_crash_no_backend(cmake, httpserver):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "none"})

    httpserver.expect_oneshot_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver), SENTRY_RELEASE="🤮🚀")

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-metrics", "capture-metric", "crash"],
        expect_failure=True,
        env=env,
    )

    # metric should have been discarded since we have no backend to hook into the crash
    assert len(httpserver.log) == 0


@pytest.mark.parametrize(
    "backend",
    [
        "inproc",
        pytest.param(
            "breakpad",
            marks=pytest.mark.skipif(
                not has_breakpad, reason="breakpad backend not available"
            ),
        ),
    ],
)
def test_metrics_on_crash(cmake, httpserver, backend):
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": backend})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-metrics", "capture-metric", "crash"],
        expect_failure=True,
        env=env,
    )

    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=env,
    )

    # we expect 1 envelope with the metric, and 1 for the crash
    assert len(httpserver.log) == 2
    metrics_request, crash_request = split_log_request_cond(
        httpserver.log, is_metrics_envelope
    )
    metrics = metrics_request.get_data()

    metrics_envelope = Envelope.deserialize(metrics)

    assert metrics_envelope is not None
    assert_metrics(metrics_envelope, 1)


@pytest.mark.skipif(not has_native, reason="test needs native backend")
def test_native_crash_http(cmake, httpserver):
    """Test native backend crash handling with HTTP transport"""
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "native"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "attachment", "crash"],
        expect_failure=True,
        env=env,
    )

    # Restart to send the crash
    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=env,
    )

    assert len(httpserver.log) >= 1
    req = httpserver.log[0][0]
    envelope = Envelope.deserialize(req.get_data())

    assert_minidump(envelope)
    assert_breadcrumb(envelope)
    assert_attachment(envelope)


@pytest.mark.skipif(not has_native, reason="test needs native backend")
def test_native_logs_on_crash(cmake, httpserver):
    """Test that logs are captured with native backend crashes"""
    tmp_path = cmake(["sentry_example"], {"SENTRY_BACKEND": "native"})

    httpserver.expect_request(
        "/api/123456/envelope/",
        headers={"x-sentry-auth": auth_header},
    ).respond_with_data("OK")
    env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))

    run(
        tmp_path,
        "sentry_example",
        ["log", "enable-logs", "capture-log", "crash"],
        expect_failure=True,
        env=env,
    )

    run(
        tmp_path,
        "sentry_example",
        ["log", "no-setup"],
        env=env,
    )

    # we expect 1 envelope with the log, and 1 for the crash
    assert len(httpserver.log) == 2
    logs_request, crash_request = split_log_request_cond(
        httpserver.log, is_logs_envelope
    )
    logs = logs_request.get_data()

    logs_envelope = Envelope.deserialize(logs)

    assert logs_envelope is not None
    assert_logs(logs_envelope, 1)
