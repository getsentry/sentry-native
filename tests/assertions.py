import datetime


def matches(actual, expected):
    return {k: v for (k, v) in actual.items() if k in expected.keys()} == expected


def assert_meta(envelope):
    event = envelope.get_event()

    expected = {
        "platform": "native",
        "environment": "Production",
        "contexts": {"runtime": {"type": "runtime", "name": "testing-runtime"}},
        "release": "test-example-release",
        "user": {"id": 42, "username": "some_name"},
        "transaction": "test-transaction",
        "tags": {"expected-tag": "some value"},
        "extra": {"extra stuff": "some value", "…unicode key…": "őá…–🤮🚀¿ 한글 테스트"},
        "sdk": {
            "name": "sentry.native",
            "version": "0.2.3",
            "packages": [
                {"name": "github:getsentry/sentry-native", "version": "0.2.3",},
            ],
        },
    }

    assert matches(event, expected)
    assert any(
        "sentry_example" in image["code_file"]
        for image in event["debug_meta"]["images"]
    )


def assert_stacktrace(envelope, inside_exception=False, check_size=False):
    event = envelope.get_event()

    parent = event["exception"] if inside_exception else event["threads"]
    frames = parent["values"][0]["stacktrace"]["frames"]
    assert isinstance(frames, list)

    if check_size:
        assert len(frames) > 0
        assert all(frame["instruction_addr"].startswith("0x") for frame in frames)


def assert_breadcrumb(envelope):
    event = envelope.get_event()

    expected = {
        "type": "http",
        "message": "debug crumb",
        "category": "example!",
        "level": "debug",
    }
    assert any(matches(b, expected) for b in event["breadcrumbs"])


def assert_attachment(envelope):
    expected = {
        "type": "attachment",
        "name": "CMakeCache.txt",
        "filename": "CMakeCache.txt",
    }
    assert any(matches(item.headers, expected) for item in envelope)


def assert_minidump(envelope):
    expected = {
        "type": "attachment",
        "name": "upload_file_minidump",
        "attachment_type": "event.minidump",
    }
    assert any(matches(item.headers, expected) for item in envelope)


def assert_timestamp(ts, now=datetime.datetime.utcnow()):
    assert ts[:11] == now.isoformat()[:11]


def assert_event(envelope):
    event = envelope.get_event()
    expected = {
        "level": "info",
        "logger": "my-logger",
        "message": {"formatted": "Hello World!"},
    }
    assert matches(event, expected)
    assert_timestamp(event["timestamp"])


def assert_crash(envelope):
    event = envelope.get_event()
    assert matches(event, {"level": "fatal"})
    # depending on the unwinder, we currently don’t get any stack frames from
    # a `ucontext`
    assert_stacktrace(envelope, inside_exception=True)
