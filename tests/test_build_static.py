import subprocess
import sys
import os
import pytest
from .conditions import has_breakpad, has_crashpad


def test_static_lib(cmake):
    tmp_path = cmake(
        ["sentry_example"],
        {
            "SENTRY_BACKEND": "none",
            "SENTRY_TRANSPORT": "none",
            "BUILD_SHARED_LIBS": "OFF",
        },
    )

    # on linux we can use `ldd` to check that we don’t link to `libsentry.so`
    if sys.platform == "linux":
        output = subprocess.check_output("ldd sentry_example", cwd=tmp_path, shell=True)
        assert b"libsentry.so" not in output

    # on windows, we use `sigcheck` to check that the exe is compiled correctly
    if sys.platform == "win32":
        output = subprocess.run(
            "sigcheck sentry_example.exe",
            cwd=tmp_path,
            shell=True,
            stdout=subprocess.PIPE,
        ).stdout
        assert (b"32-bit" if os.environ.get("TEST_X86") else b"64-bit") in output
    # similarly, we use `file` on linux
    if sys.platform == "linux":
        output = subprocess.check_output(
            "file sentry_example", cwd=tmp_path, shell=True
        )
        assert (
            b"ELF 32-bit" if os.environ.get("TEST_X86") else b"ELF 64-bit"
        ) in output


@pytest.mark.skipif(not has_crashpad, reason="test needs crashpad backend")
def test_static_crashpad(cmake):
    tmp_path = cmake(
        ["sentry_example"],
        {
            "SENTRY_BACKEND": "crashpad",
            "SENTRY_TRANSPORT": "none",
            "BUILD_SHARED_LIBS": "OFF",
        },
    )


@pytest.mark.skipif(not has_breakpad, reason="test needs crashpad backend")
def test_static_breakpad(cmake):
    tmp_path = cmake(
        ["sentry_example"],
        {
            "SENTRY_BACKEND": "breakpad",
            "SENTRY_TRANSPORT": "none",
            "BUILD_SHARED_LIBS": "OFF",
        },
    )
