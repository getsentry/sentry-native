import time

import pytest

from . import adb, run
from .conditions import has_native, is_android

pytestmark = pytest.mark.skipif(
    not is_android,
    reason="Tests need Android",
)


@pytest.mark.skipif(
    not has_native,
    reason="Tests need the native backend enabled",
)
@pytest.mark.skipif(
    0 < is_android < 23,
    reason="Android native backend test needs API 23+ (process_vm_readv)",
)
def test_native_android(cmake):
    database_path = "/data/local/tmp/.sentry-native"

    def find_minidumps(timeout=10):
        deadline = time.time() + timeout
        last_result = None
        while last_result is None or time.time() < deadline:
            last_result = adb(
                "shell",
                f'for f in {database_path}/*.dmp; do [ -f "$f" ] && echo "$f"; done',
                capture_output=True,
                text=True,
                check=False,
            )
            if last_result.returncode == 0 and last_result.stdout.strip():
                return [
                    line.strip()
                    for line in last_result.stdout.splitlines()
                    if line.strip()
                ]
            time.sleep(0.5)
        if last_result is not None:
            print(last_result.stdout)
            print(last_result.stderr)
        return []

    tmp_path = cmake(
        ["sentry_example"], {"SENTRY_BACKEND": "native", "SENTRY_TRANSPORT": "none"}
    )

    adb("shell", f"rm -rf {database_path}", check=True)
    assert find_minidumps(timeout=0) == []

    try:
        run(tmp_path, "sentry_example", ["log", "crash"], expect_failure=True)

        minidumps = find_minidumps()
        assert minidumps, "native backend should create a minidump on Android"

        local_minidump = tmp_path / "android.dmp"
        adb("pull", minidumps[0], str(local_minidump), check=True, capture_output=True)
        assert local_minidump.stat().st_size > 0
    finally:
        adb("shell", f"rm -rf {database_path}", check=False)
