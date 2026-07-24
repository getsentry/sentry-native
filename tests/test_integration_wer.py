import ctypes
from ctypes import wintypes
import os
import subprocess
import sys

from pathlib import Path

import pytest

from . import run
from .assertions import wait_for
from .conditions import has_breakpad, has_crashpad, has_native, is_qemu

pytestmark = [
    pytest.mark.skipif(
        sys.platform != "win32" or bool(os.environ.get("TEST_MINGW")),
        reason="WER integration tests are only available in MSVC Windows builds",
    ),
    pytest.mark.with_wer,
]

S_OK = 0

E_STORE_USER_ARCHIVE = 0
E_STORE_USER_QUEUE = 1
E_STORE_MACHINE_ARCHIVE = 2
E_STORE_MACHINE_QUEUE = 3


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
                    reason="breakpad swallows the exception",
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
                    reason="crashpad handler terminates the process",
                    strict=True,
                ),
            ],
        ),
        pytest.param(
            "native",
            marks=pytest.mark.skipif(
                not has_native or is_qemu, reason="native backend not available"
            ),
            id="native",
        ),
    ],
)
def test_wer_custom_metadata(cmake, backend):
    tmp_path = cmake(
        ["sentry_example"],
        {
            "SENTRY_BACKEND": backend,
            "SENTRY_TRANSPORT": "none",
            "SENTRY_INTEGRATION_WER": "ON",
        },
    )

    completed = run(
        tmp_path,
        "sentry_example",
        ["e2e-test", "crash"],
        expect_failure=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )

    test_id = None
    for line in completed.stdout.splitlines():
        if line.startswith("TEST_ID:"):
            test_id = line.removeprefix("TEST_ID:")
            break
    assert test_id, completed.stdout

    report_path, report = wait_for_wer_report(WerStore(), test_id)
    assert report_path.name == "Report.wer"
    assert "expected-tag" in report
    assert "some value" in report
    assert "not-expected-tag" not in report
