import ctypes
from ctypes import wintypes
import base64
import os
import platform
import re
import shutil
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
from .conditions import has_breakpad, has_crashpad, has_native, is_qemu, is_x86

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


def assert_sentry_event(httpserver, backend, crash_arg, exception_code=None):
    assert len(httpserver.log) >= 1

    if backend == "crashpad":
        assert httpserver.log[0][0].path == "/api/123456/minidump/"
        attachments = assert_crashpad_upload(httpserver.log[0][0])
        assert attachments.event["event_id"]
        assert attachments.event["tags"]["test.initial-tag"] == "initial-value"
        assert attachments.event["contexts"]["initial"]["foo"] == "bar"
        return attachments.event

    envelope = Envelope.deserialize(httpserver.log[0][0].get_data())
    event = envelope.get_event()
    assert event is not None
    assert event["event_id"]
    assert event["tags"]["test.initial-tag"] == "initial-value"
    assert event["contexts"]["initial"]["foo"] == "bar"
    assert_event_meta(event, integrations=[backend, "wer"])

    if backend == "inproc":
        assert_inproc_crash(envelope)
    elif backend == "breakpad":
        assert_minidump(envelope)
        assert_breakpad_crash(envelope)
    elif backend == "native":
        assert_native_crash(envelope, exception_code=exception_code)

    return event


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


def run_wer_crash(
    cmake,
    backend,
    crash_arg,
    httpserver=None,
    appx=False,
    winui=False,
    exception_code=None,
):
    build_options = {
        "SENTRY_BACKEND": backend,
        "SENTRY_INTEGRATION_WER": "ON",
    }
    if httpserver is None:
        build_options["SENTRY_TRANSPORT"] = "none"

    if appx:
        target = "sentry_test_appx"
    elif winui:
        target = "sentry_test_winui"
    else:
        target = "sentry_test_integration"
    tmp_path = cmake(
        ["sentry"] if winui else [target],
        build_options,
    )

    if appx:
        register_appx_package(tmp_path)
    if winui:
        build_winui(tmp_path)

    try:
        env = None
        sentry_event = None
        sentry_wer_report_id = None
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
        run_args.append("initial-scope")
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
                encoding="utf-8",
                errors="replace",
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
                    encoding="utf-8",
                    errors="replace",
                )
                if backend in ("breakpad", "inproc"):
                    run(tmp_path, target, ["no-setup"], env=env)

            assert waiting.result
            sentry_event = assert_sentry_event(
                httpserver, backend, crash_arg, exception_code
            )
            if backend == "native":
                sentry_wer_report_id = sentry_event["contexts"]["wer"]["report_id"]
                assert sentry_wer_report_id

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

        if sentry_wer_report_id is not None:
            report_ids = {
                line.partition("=")[2]
                for line in report.splitlines()
                if line.startswith(("ReportIdentifier=", "IntegratorReportIdentifier="))
            }
            assert sentry_wer_report_id in report_ids

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
                    run=False,  # save CI time
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
                    run=False,  # save CI time
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

    assert "test.initial-tag" in report
    assert "initial-value" in report
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
                    run=False,  # save CI time
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
                    run=False,  # save CI time
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
                    run=False,  # save CI time
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
                    run=False,  # save CI time
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
@pytest.mark.parametrize("crash_arg", ["crash", "fastfail"])
def test_wer_appx(cmake, httpserver, crash_arg):
    report = run_wer_crash(cmake, "native", crash_arg, httpserver, appx=True)

    assert "test.id" in report.lower()


STATUS_ACCESS_VIOLATION = 0xC0000005
STATUS_STOWED_EXCEPTION = 0xC000027B

winui_fixture_path = Path(__file__).parent / "fixtures" / "winui"


def find_msbuild():
    vswhere = (
        Path(os.environ.get("ProgramFiles(x86)", ""))
        / "Microsoft Visual Studio"
        / "Installer"
        / "vswhere.exe"
    )
    if not vswhere.exists():
        return None
    output = subprocess.run(
        [
            vswhere,
            "-latest",
            "-requires",
            "Microsoft.Component.MSBuild",
            "-find",
            r"MSBuild\**\Bin\MSBuild.exe",
        ],
        capture_output=True,
        text=True,
    ).stdout.splitlines()
    return output[0] if output else None


def build_winui(tmp_path):
    msbuild = find_msbuild()
    if msbuild is None:
        pytest.skip("MSBuild is not installed")

    project_path = tmp_path / "winui"
    shutil.copytree(winui_fixture_path, project_path, dirs_exist_ok=True)
    if is_x86:
        arch = "Win32"
    elif platform.machine() == "ARM64":
        arch = "ARM64"
    else:
        arch = "x64"
    subprocess.run(
        [
            msbuild,
            project_path / "sentry_test_winui.vcxproj",
            "-restore",
            "-nologo",
            "-nodeReuse:false",
            "-verbosity:minimal",
            "-p:Configuration=Debug",
            f"-p:Platform={arch}",
            "-p:IgnoreWarnIntDirInTempDetected=true",
            f"-p:OutDir={tmp_path}\\",
            f"-p:SentryIncludeDir={Path(__file__).parent.parent / 'include'}",
            f"-p:SentryLib={tmp_path / 'sentry.lib'}",
        ],
        env=dict(os.environ, MSBUILDDISABLENODEREUSE="1"),
        check=True,
    )


@pytest.mark.skipif(not has_native or is_qemu, reason="native backend not available")
@pytest.mark.parametrize(
    "crash_arg,exception_code",
    [
        pytest.param(
            "dispatcher-crash", STATUS_ACCESS_VIOLATION, id="dispatcher-crash"
        ),
        pytest.param(
            "dispatcher-exception",
            STATUS_STOWED_EXCEPTION,
            id="dispatcher-exception",
        ),
        pytest.param("loaded-crash", STATUS_ACCESS_VIOLATION, id="loaded-crash"),
        pytest.param(
            "loaded-exception", STATUS_STOWED_EXCEPTION, id="loaded-exception"
        ),
    ],
)
def test_wer_winui(cmake, httpserver, crash_arg, exception_code):
    report = run_wer_crash(
        cmake,
        "native",
        crash_arg,
        httpserver,
        winui=True,
        exception_code=exception_code,
    )

    assert len(httpserver.log) == 1
    assert "test.initial-tag" in report
    assert "initial-value" in report
    assert "expected-tag" in report
    assert "some value" in report
    assert "not-expected-tag" not in report
