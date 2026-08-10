import ctypes
from ctypes import wintypes
import base64
import os
import re
import subprocess
import sys

from pathlib import Path

import pytest

from . import Envelope, make_dsn, run
from .assertions import (
    assert_breakpad_crash,
    assert_crashpad_upload,
    assert_event_meta,
    assert_inproc_crash,
    assert_minidump,
    assert_native_crash,
    wait_for,
)
from .conditions import has_breakpad, has_crashpad, has_native, is_qemu

pytestmark = [
    pytest.mark.skipif(
        sys.platform != "win32" or bool(os.environ.get("TEST_MINGW")),
        reason="WER integration tests are only available in MSVC Windows builds",
    ),
    pytest.mark.with_wer,
]

S_OK = 0
APPX_PACKAGE_NAME = "SentryNativeExample"
APPX_ACTIVITY_ID_RE = re.compile(r"ActivityId[:\s]+([0-9a-fA-F-]{36})", re.IGNORECASE)

E_STORE_USER_ARCHIVE = 0
E_STORE_USER_QUEUE = 1
E_STORE_MACHINE_ARCHIVE = 2
E_STORE_MACHINE_QUEUE = 3

PNG_1X1_TRANSPARENT = base64.b64decode(
    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADgg"
    "GBlN2eJwAAAABJRU5ErkJggg=="
)


class WerStore:
    def __init__(self):
        self._wer = ctypes.WinDLL("wer.dll")
        self._wer.WerStoreOpen.argtypes = [
            ctypes.c_int,
            ctypes.POINTER(wintypes.HANDLE),
        ]
        self._wer.WerStoreOpen.restype = ctypes.c_long
        self._wer.WerStoreClose.argtypes = [wintypes.HANDLE]
        self._wer.WerStoreClose.restype = None
        self._wer.WerStoreGetFirstReportKey.argtypes = [
            wintypes.HANDLE,
            ctypes.POINTER(ctypes.c_wchar_p),
        ]
        self._wer.WerStoreGetFirstReportKey.restype = ctypes.c_long
        self._wer.WerStoreGetNextReportKey.argtypes = [
            wintypes.HANDLE,
            ctypes.POINTER(ctypes.c_wchar_p),
        ]
        self._wer.WerStoreGetNextReportKey.restype = ctypes.c_long
        self._wer.WerFreeString.argtypes = [ctypes.c_wchar_p]
        self._wer.WerFreeString.restype = None

    def report_dirs(self):
        for store_type in (
            E_STORE_USER_ARCHIVE,
            E_STORE_USER_QUEUE,
            E_STORE_MACHINE_ARCHIVE,
            E_STORE_MACHINE_QUEUE,
        ):
            handle = wintypes.HANDLE()
            hr = self._wer.WerStoreOpen(store_type, ctypes.byref(handle))
            if hr != S_OK:
                continue

            try:
                key = ctypes.c_wchar_p()
                hr = self._wer.WerStoreGetFirstReportKey(handle, ctypes.byref(key))
                while hr == S_OK and key.value:
                    report_dir = key.value
                    self._wer.WerFreeString(key)
                    yield Path(report_dir)

                    key = ctypes.c_wchar_p()
                    hr = self._wer.WerStoreGetNextReportKey(handle, ctypes.byref(key))
            finally:
                self._wer.WerStoreClose(handle)


def wait_for_wer_report(store, test_id):
    seen_report_dirs = 0
    readable_reports = 0
    matching_report = None

    def find_report():
        nonlocal matching_report
        nonlocal seen_report_dirs
        nonlocal readable_reports

        for report_dir in store.report_dirs():
            seen_report_dirs += 1
            report_path = report_dir / "Report.wer"
            try:
                report = report_path.read_text(encoding="utf-16-le")
            except OSError:
                continue

            readable_reports += 1
            report_lower = report.lower()
            if "test.id" in report_lower and test_id in report:
                matching_report = (report_path, report)
                return True

        return False

    if wait_for(find_report):
        return matching_report

    details = [
        f"searched {seen_report_dirs} WER report directories",
        f"read {readable_reports} Report.wer files",
    ]
    pytest.fail(
        f"WER report with test.id={test_id} was not found ({', '.join(details)})"
    )


def assert_sentry_event(httpserver, backend, crash_arg):
    assert len(httpserver.log) >= 1

    if backend == "crashpad":
        assert httpserver.log[0][0].path == "/api/123456/minidump/"
        attachments = assert_crashpad_upload(httpserver.log[0][0])
        assert attachments.event["event_id"]
        return

    envelope = Envelope.deserialize(httpserver.log[0][0].get_data())
    event = envelope.get_event()
    assert event is not None
    assert event["event_id"]
    assert_event_meta(event, integrations=[backend, "wer"])

    if backend == "inproc":
        assert_inproc_crash(envelope)
    elif backend == "breakpad":
        assert_minidump(envelope)
        assert_breakpad_crash(envelope)
    elif backend == "native":
        assert_native_crash(envelope)


def powershell(command, check=True):
    completed = subprocess.run(
        [
            "powershell.exe",
            "-NoLogo",
            "-NoProfile",
            "-NonInteractive",
            "-ExecutionPolicy",
            "Bypass",
            "-Command",
            command,
        ],
        capture_output=True,
        text=True,
    )
    if check and completed.returncode != 0:
        pytest.fail(
            f"PowerShell command failed with exit code {completed.returncode}\n"
            f"Command:\n{command}\n"
            f"stdout:\n{completed.stdout}\n"
            f"stderr:\n{completed.stderr}"
        )
    return completed


def remove_appx_package():
    powershell(
        f"Get-AppxPackage -Name {APPX_PACKAGE_NAME} | Remove-AppxPackage",
        check=False,
    )


def ensure_appx_assets(tmp_path):
    assets = tmp_path / "Assets"
    assets.mkdir(exist_ok=True)
    for name in ("StoreLogo.png", "Square150x150Logo.png", "Square44x44Logo.png"):
        (assets / name).write_bytes(PNG_1X1_TRANSPARENT)


def register_appx_package(tmp_path):
    ensure_appx_assets(tmp_path)
    remove_appx_package()

    manifest = tmp_path / "AppxManifest.xml"
    completed = powershell(
        "Add-AppxPackage " f"-Register '{manifest}' " f"-ExternalLocation '{tmp_path}'",
        check=False,
    )
    if completed.returncode != 0:
        output = "\n".join(filter(None, [completed.stdout, completed.stderr]))
        match = APPX_ACTIVITY_ID_RE.search(output)
        appx_log = ""
        if match:
            log_result = powershell(
                f"Get-AppxLog -ActivityID '{match.group(1)}'",
                check=False,
            )
            appx_log = "\nAppX deployment log:\n" + "\n".join(
                filter(None, [log_result.stdout, log_result.stderr])
            )
        pytest.fail(
            "Add-AppxPackage -Register -ExternalLocation failed"
            f"\nstdout:\n{completed.stdout}"
            f"\nstderr:\n{completed.stderr}"
            f"{appx_log}"
        )


def run_wer_crash(cmake, backend, crash_arg, httpserver=None, appx=False):
    build_options = {
        "SENTRY_BACKEND": backend,
        "SENTRY_INTEGRATION_WER": "ON",
    }
    if httpserver is None:
        build_options["SENTRY_TRANSPORT"] = "none"

    target = "sentry_example_appx" if appx else "sentry_example"
    tmp_path = cmake(
        [target],
        build_options,
    )

    if appx:
        register_appx_package(tmp_path)

    try:
        env = None
        if httpserver is not None:
            env = dict(os.environ, SENTRY_DSN=make_dsn(httpserver))
            if backend == "crashpad":
                httpserver.expect_oneshot_request(
                    "/api/123456/minidump/"
                ).respond_with_data("OK")
            else:
                httpserver.expect_oneshot_request(
                    "/api/123456/envelope/"
                ).respond_with_data("OK")

        run_args = ["e2e-test"]
        if appx:
            run_args.append("appx")
        run_args.append(crash_arg)
        if backend == "crashpad":
            run_args.append("crashpad-wait-for-upload")

        if httpserver is None:
            completed = run(
                tmp_path,
                target,
                run_args,
                expect_failure=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
        else:
            with httpserver.wait(timeout=10) as waiting:
                completed = run(
                    tmp_path,
                    target,
                    run_args,
                    expect_failure=True,
                    env=env,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    text=True,
                )
                if backend in ("breakpad", "inproc"):
                    run(tmp_path, target, ["no-setup"], env=env)

            assert waiting.result
            assert_sentry_event(httpserver, backend, crash_arg)

        if appx:
            assert "PACKAGE_IDENTITY:present" in completed.stdout

        test_id = None
        for line in completed.stdout.splitlines():
            if line.startswith("TEST_ID:"):
                test_id = line.removeprefix("TEST_ID:")
                break
        assert test_id, completed.stdout

        report_path, report = wait_for_wer_report(WerStore(), test_id)
        assert report_path.name == "Report.wer"

        return report
    finally:
        if appx:
            remove_appx_package()


@pytest.mark.parametrize(
    "backend",
    [
        "none",
        "inproc",
        pytest.param(
            "breakpad",
            marks=[
                pytest.mark.skipif(
                    not has_breakpad or is_qemu, reason="breakpad backend not available"
                ),
                pytest.mark.xfail(
                    reason="breakpad swallows SEH exceptions",
                    strict=True,
                ),
            ],
        ),
        pytest.param(
            "crashpad",
            marks=[
                pytest.mark.skipif(
                    not has_crashpad, reason="crashpad backend not available"
                ),
                pytest.mark.xfail(
                    reason="crashpad terminates the process",
                    strict=True,
                ),
            ],
        ),
        pytest.param(
            "native",
            marks=pytest.mark.skipif(
                not has_native or is_qemu, reason="native backend not available"
            ),
        ),
    ],
)
def test_wer_custom_metadata(cmake, backend):
    report = run_wer_crash(cmake, backend, "crash")

    assert "expected-tag" in report
    assert "some value" in report
    assert "not-expected-tag" not in report


@pytest.mark.parametrize(
    "backend,crash_arg",
    [
        pytest.param("inproc", "crash"),
        pytest.param(
            "inproc",
            "fastfail",
            marks=pytest.mark.xfail(
                reason="inproc does not capture WER exceptions",
                strict=True,
            ),
        ),
        pytest.param(
            "breakpad",
            "crash",
            marks=[
                pytest.mark.skipif(
                    not has_breakpad or is_qemu, reason="breakpad backend not available"
                ),
                pytest.mark.xfail(
                    reason="breakpad swallows SEH exceptions",
                    strict=True,
                ),
            ],
        ),
        pytest.param(
            "breakpad",
            "fastfail",
            marks=[
                pytest.mark.skipif(
                    not has_breakpad or is_qemu, reason="breakpad backend not available"
                ),
                pytest.mark.xfail(
                    reason="breakpad does not capture WER exceptions",
                    strict=True,
                ),
            ],
        ),
        pytest.param(
            "crashpad",
            "crash",
            marks=[
                pytest.mark.skipif(
                    not has_crashpad, reason="crashpad backend not available"
                ),
                pytest.mark.xfail(
                    reason="crashpad terminates the process",
                    strict=True,
                ),
            ],
        ),
        pytest.param(
            "crashpad",
            "fastfail",
            marks=[
                pytest.mark.skipif(
                    not has_crashpad, reason="crashpad backend not available"
                ),
                pytest.mark.xfail(
                    reason="crashpad terminates the process",
                    strict=True,
                ),
            ],
        ),
        pytest.param(
            "native",
            "crash",
            marks=pytest.mark.skipif(
                not has_native or is_qemu, reason="native backend not available"
            ),
        ),
        pytest.param(
            "native",
            "fastfail",
            marks=pytest.mark.skipif(
                not has_native or is_qemu, reason="native backend not available"
            ),
        ),
    ],
)
def test_wer_compatibility(cmake, httpserver, backend, crash_arg):
    report = run_wer_crash(cmake, backend, crash_arg, httpserver)

    assert "test.id" in report.lower()


@pytest.mark.skipif(not has_native or is_qemu, reason="native backend not available")
def test_wer_appx(cmake, httpserver):
    report = run_wer_crash(cmake, "native", "fastfail", httpserver, appx=True)

    assert "test.id" in report.lower()
