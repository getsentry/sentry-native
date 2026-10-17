import os
import subprocess
import sys
from pathlib import Path

import pytest

from .build_config import get_platform_cmake_args
from .conditions import has_crashpad, has_native, is_android, is_arm32, is_qemu, is_wine


@pytest.mark.skipif(
    is_android
    or is_arm32
    or is_qemu
    or is_wine
    or sys.platform not in ("linux", "darwin", "win32"),
    reason="CTest executes the unit binary on the build host",
)
@pytest.mark.parametrize(
    "backend",
    [
        "none",
        pytest.param(
            "native",
            marks=pytest.mark.skipif(not has_native, reason="requires native backend"),
        ),
        pytest.param(
            "crashpad",
            marks=pytest.mark.skipif(not has_crashpad, reason="requires Crashpad"),
        ),
    ],
)
def test_ctest(tmp_path, backend):
    source = Path(__file__).resolve().parent.parent
    build = tmp_path / "build with spaces"
    build.mkdir()
    configure = [
        "cmake",
        str(source),
        *get_platform_cmake_args(),
        "-DCMAKE_BUILD_TYPE=RelWithDebInfo",
        "-DSENTRY_BUILD_TESTS=ON",
        f"-DSENTRY_BACKEND={backend}",
    ]
    subprocess.run(configure, check=True, cwd=build)
    # Do not supply a common runtime output directory: that would mask missing
    # helpers in a normal CMake build. Respect CMAKE_BUILD_PARALLEL_LEVEL.
    subprocess.run(
        [
            "cmake",
            "--build",
            str(build),
            "--config",
            "RelWithDebInfo",
            "--target",
            "sentry_test_unit",
        ],
        check=True,
    )
    ctest = ["ctest", "-C", "RelWithDebInfo", "--output-on-failure"]
    if backend == "native":
        # Exercise native helper discovery and concurrent filesystem fixtures;
        ctest += [
            "-R",
            "sentry/(basic_function_transport|path_basics|cache_max_size|installation_id|multiple_inits)$",
        ]
    subprocess.run(
        [*ctest, "--parallel", os.environ.get("CTEST_PARALLEL_LEVEL", "2")],
        check=True,
        cwd=build,
    )
    if backend == "none":
        # Prefixes are argv lists and may contain paths/arguments with spaces.
        prefix = tmp_path / "test prefix.py"
        receipt = tmp_path / "prefix calls.txt"
        prefix.write_text(
            "import pathlib, subprocess, sys\n"
            "path = pathlib.Path(sys.argv[1])\n"
            "with path.open('a') as f: f.write(sys.argv[2] + '\\n')\n"
            "sys.exit(subprocess.call(sys.argv[3:]))\n"
        )
        subprocess.run(
            [
                *configure,
                f"-DCMAKE_CROSSCOMPILING_EMULATOR={sys.executable};{prefix};{receipt};emulator argument",
                f"-DCMAKE_TEST_LAUNCHER={sys.executable};{prefix};{receipt};launcher argument",
                "-DCMAKE_POLICY_DEFAULT_CMP0158=OLD",
            ],
            check=True,
            cwd=build,
        )
        subprocess.run([*ctest, "-R", "sentry/path_basics$"], check=True, cwd=build)
        calls = receipt.read_text().splitlines()
        assert "emulator argument" in calls
        # TEST_LAUNCHER was introduced in CMake 3.29.
        version = subprocess.check_output(["cmake", "--version"], text=True)
        major, minor = map(int, version.split()[2].split(".")[:2])
        if (major, minor) >= (3, 29):
            assert calls == ["launcher argument", "emulator argument"]
    if backend != "native":
        subprocess.run(
            [*configure, "-DSENTRY_CTEST_INDIVIDUAL=OFF"], check=True, cwd=build
        )
        subprocess.run(ctest, check=True, cwd=build)


@pytest.mark.parametrize("exit_code", [0, 7])
def test_ctest_working_directory(tmp_path, exit_code):
    script = Path(__file__).parent / "unit" / "run_test.cmake"
    probe = tmp_path / "probe.py"
    receipt = tmp_path / "working directories.txt"
    probe.write_text(
        "import pathlib, sys\n"
        "cwd = pathlib.Path.cwd()\n"
        "assert (cwd / '.ctest-fixture').is_file()\n"
        "with pathlib.Path(sys.argv[1]).open('a') as f: f.write(str(cwd) + '\\n')\n"
        f"sys.exit({exit_code})\n"
    )
    for _ in range(2):
        result = subprocess.run(
            [
                "cmake",
                f"-DTEST_EXECUTABLE={sys.executable}",
                f"-DTEST_LAUNCHER={sys.executable};{probe};{receipt}",
                f"-DTEST_WORKING_ROOT={tmp_path / 'fixtures with spaces'}",
                "-DTEST_NAME=probe",
                "-P",
                str(script),
            ],
            capture_output=True,
            text=True,
        )
        assert (result.returncode == 0) == (exit_code == 0)
        if exit_code:
            assert "failed (7)" in result.stderr
    directories = [Path(line) for line in receipt.read_text().splitlines()]
    assert len(set(directories)) == 2
    assert all(path.exists() == bool(exit_code) for path in directories)
