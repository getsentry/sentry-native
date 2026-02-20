import json
import os
import shutil
import subprocess
import sys
import platform
from pathlib import Path

import pytest

from .build_config import (
    get_android_config,
    get_platform_cmake_args,
    get_cflags,
    get_tsan_env,
)


class CMake:
    def __init__(self, factory):
        self.runs = dict()
        self.factory = factory

    def compile(self, targets, options=None, cflags=None):
        # We build in tmp for all of our tests. Disable the warning MSVC generates to not clutter the build logs.
        if cflags is None:
            cflags = []
        if sys.platform == "win32":
            os.environ["IgnoreWarnIntDirInTempDetected"] = "True"

        if options is None:
            options = dict()
        key = (
            ";".join(targets),
            ";".join(f"{k}={v}" for k, v in options.items()),
        )

        # cache the build configuration
        if key not in self.runs:
            cwd = self.factory.mktemp("cmake")
            self.runs[key] = cwd
            cmake(cwd, targets, options, cflags)

        build_tmp_path = self.runs[key]

        # ensure that there are no left-overs from previous runs
        shutil.rmtree(build_tmp_path / ".sentry-native", ignore_errors=True)

        # Inject a sub-path into the temporary build directory as the CWD for all tests to verify UTF-8 path handling.
        if os.environ.get("UTF8_TEST_CWD"):
            # this is Thai and translates to "this is a test directory"
            utf8_subpath = build_tmp_path / "นี่คือไดเร็กทอรีทดสอบ"
            shutil.rmtree(utf8_subpath, ignore_errors=True)
            shutil.copytree(build_tmp_path, utf8_subpath, dirs_exist_ok=True)
            return utf8_subpath

        return build_tmp_path

    def destroy(self):
        sourcedir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
        coveragedir = os.path.join(sourcedir, "coverage")
        shutil.rmtree(coveragedir, ignore_errors=True)
        os.mkdir(coveragedir)

        if "llvm-cov" in os.environ.get("RUN_ANALYZER", ""):
            for i, d in enumerate(self.runs.values()):
                # first merge the raw profiling runs
                files = [f for f in os.listdir(d) if f.endswith(".profraw")]
                if len(files) == 0:
                    continue
                cmd = [
                    "llvm-profdata",
                    "merge",
                    "-sparse",
                    "-o=sentry.profdata",
                    *files,
                ]
                print("{} > {}".format(d, " ".join(cmd)))
                subprocess.run(cmd, cwd=d)

                # then export lcov from the profiling data, since this needs access
                # to the object files, we need to do it per-test
                objects = [
                    "sentry_example",
                    "sentry_test_unit",
                    "libsentry.dylib" if sys.platform == "darwin" else "libsentry.so",
                ]
                cmd = [
                    "llvm-cov",
                    "export",
                    "-format=lcov",
                    "-instr-profile=sentry.profdata",
                    "--ignore-filename-regex=(external|vendor|tests|examples)",
                    *[f"-object={o}" for o in objects if d.joinpath(o).exists()],
                ]
                lcov = os.path.join(coveragedir, f"run-{i}.lcov")
                with open(lcov, "w") as lcov_file:
                    print("{} > {} > {}".format(d, " ".join(cmd), lcov))
                    subprocess.run(cmd, stdout=lcov_file, cwd=d)

        if "kcov" in os.environ.get("RUN_ANALYZER", ""):
            coverage_dirs = [
                d
                for d in [d.joinpath("coverage") for d in self.runs.values()]
                if d.exists()
            ]
            if len(coverage_dirs) > 0:
                subprocess.run(
                    [
                        "kcov",
                        "--clean",
                        "--merge",
                        coveragedir,
                        *coverage_dirs,
                    ]
                )


def cmake(cwd, targets, options=None, cflags=None):
    if cflags is None:
        cflags = []
    __tracebackhide__ = True
    if options is None:
        options = {}
    if os.environ.get("USE_CCACHE"):
        options.update(
            {
                "CMAKE_C_COMPILER_LAUNCHER": "ccache",
                "CMAKE_CXX_COMPILER_LAUNCHER": "ccache",
            }
        )
    options.update(
        {
            "CMAKE_RUNTIME_OUTPUT_DIRECTORY": cwd,
            "CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG": cwd,
            "CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE": cwd,
        }
    )

    options.update(get_android_config())

    source_dir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))

    cmake = ["cmake"]
    if "scan-build" in os.environ.get("RUN_ANALYZER", ""):
        cc = os.environ.get("CC")
        cxx = os.environ.get("CXX")
        cmake = [
            "scan-build",
            *(["--use-cc", cc] if cc else []),
            *(["--use-c++", cxx] if cxx else []),
            "--status-bugs",
            "--exclude",
            os.path.join(source_dir, "external"),
            "cmake",
        ]

    config_cmd = cmake.copy()

    if os.environ.get("VS_GENERATOR_TOOLSET") == "ClangCL":
        configure_clang_cl(config_cmd)

    for key, value in options.items():
        config_cmd.append("-D{}={}".format(key, value))

    config_cmd.extend(get_platform_cmake_args())

    tsan_opts = get_tsan_env()
    if tsan_opts:
        os.environ["TSAN_OPTIONS"] = tsan_opts

    cflags.extend(get_cflags())
    if "llvm-cov" in os.environ.get("RUN_ANALYZER", ""):
        configure_llvm_cov(config_cmd)
    env = dict(os.environ)
    env["CFLAGS"] = env["CXXFLAGS"] = " ".join(cflags)
    if env.get("USE_CCACHE"):
        # Each pytest run builds in a new temp directory. Paths are normalized
        # relative to the build dir and CWD hashing is skipped to allow ccache
        # hits across runs.
        env.setdefault("CCACHE_BASEDIR", str(cwd))
        env.setdefault("CCACHE_NOHASHDIR", "true")

    config_cmd.append(source_dir)

    print("\n{} > {}".format(cwd, " ".join(config_cmd)), flush=True)
    try:
        result = subprocess.run(
            config_cmd, cwd=cwd, env=env, check=True, capture_output=True, text=True
        )
        sys.stdout.write(result.stdout)
        sys.stderr.write(result.stderr or "")
    except subprocess.CalledProcessError as e:
        if "Could NOT find ZLIB" in e.stderr:
            pytest.skip("ZLIB not found")
        else:
            sys.stdout.write(e.stdout)
            sys.stderr.write(e.stderr or "")
            raise pytest.fail.Exception("cmake configure failed") from None

    # CodeChecker invocations and options are documented here:
    # https://github.com/Ericsson/codechecker/blob/master/docs/analyzer/user_guide.md

    buildcmd = [*cmake, "--build", "."]
    for target in targets:
        buildcmd.extend(["--target", target])
    buildcmd.append("--parallel")
    if "CMAKE_BUILD_TYPE" in options:
        buildcmd.extend(["--config", options["CMAKE_BUILD_TYPE"]])
    if "code-checker" in os.environ.get("RUN_ANALYZER", ""):
        buildcmd = [
            "codechecker",
            "log",
            "--output",
            "compilation.json",
            "--build",
            " ".join(buildcmd),
        ]

    print("{} > {}".format(cwd, " ".join(buildcmd)), flush=True)
    try:
        subprocess.run(buildcmd, cwd=cwd, env=env, check=True)
    except subprocess.CalledProcessError:
        raise pytest.fail.Exception("cmake build failed") from None

    # check if the DLL and EXE artifacts contain version-information
    if platform.system() == "Windows":
        from tests.win_utils import check_binary_version

        check_binary_version(Path(cwd) / "sentry.dll")
        check_binary_version(Path(cwd) / "crashpad_wer.dll")
        check_binary_version(Path(cwd) / "crashpad_handler.exe")

    if "code-checker" in os.environ.get("RUN_ANALYZER", ""):
        # For whatever reason, the compilation summary contains duplicate entries,
        # one with the correct absolute path, and the other one just with the basename,
        # which would fail.
        with open(os.path.join(cwd, "compilation.json")) as f:
            compilation = json.load(f)
            compilation = list(filter(lambda c: c["file"].startswith("/"), compilation))
        with open(os.path.join(cwd, "compilation.json"), "w") as f:
            json.dump(compilation, f)

        disable = [
            "readability-magic-numbers",
            "cppcoreguidelines-avoid-magic-numbers",
            "readability-else-after-return",
        ]
        disables = ["--disable={}".format(d) for d in disable]
        checkcmd = [
            "codechecker",
            "check",
            "--jobs",
            str(os.cpu_count()),
            # NOTE: The clang version on CI does not support CTU :-(
            # Also, when testing locally, CTU spews a ton of (possibly) false positives
            # "--ctu-all",
            # TODO: we currently get >300 reports with `enable-all`
            # "--enable-all",
            *disables,
            "--print-steps",
            "--ignore",
            os.path.join(source_dir, ".codechecker-ignore"),
            "--logfile",
            "compilation.json",
        ]
        print("{} > {}".format(cwd, " ".join(checkcmd)), flush=True)
        subprocess.run(checkcmd, cwd=cwd, check=True)

    if os.environ.get("ANDROID_API"):
        # copy the output to the android image via adb
        subprocess.run(
            [
                "{}/platform-tools/adb".format(os.environ["ANDROID_HOME"]),
                "push",
                "./",
                "/data/local/tmp",
            ],
            cwd=cwd,
            check=True,
        )


def configure_clang_cl(config_cmd: list[str]):
    config_cmd.append("-G Visual Studio 17 2022")
    config_cmd.append("-A x64")
    config_cmd.append("-T ClangCL")


def configure_llvm_cov(config_cmd: list[str]):
    if False and os.environ.get("VS_GENERATOR_TOOLSET") == "ClangCL":
        # for clang-cl in CI we have to use `--coverage` rather than `fprofile-instr-generate` and provide an
        # architecture-specific profiling library for it work:
        # TODO: This currently doesn't work due to https://gitlab.kitware.com/cmake/cmake/-/issues/24025
        #       The issue describes a behavior where generated object-name is specified via `/fo` using a target
        #       directory, rather than a file-name (this is CMake behavior). While the `clang-cl` suggest that this
        #       is supported the flag produces `.gcda` and `.gcno` files, which have no relation with the object-
        #       file and which leads to failure to accumulate coverage data.
        #       This would have to be fixed in clang-cl: https://github.com/llvm/llvm-project/issues/87304
        #       Let's leave this in here for posterity, it would be great to get coverage analysis on Windows.
        flags = "--coverage"
        profile_lib = "clang_rt.profile-x86_64.lib"
        config_cmd.append(f"-DCMAKE_EXE_LINKER_FLAGS='{profile_lib}'")
        config_cmd.append(f"-DCMAKE_SHARED_LINKER_FLAGS='{profile_lib}'")
        config_cmd.append(f"-DCMAKE_MODULE_LINKER_FLAGS='{profile_lib}'")
    else:
        flags = "-fprofile-instr-generate -fcoverage-mapping"
    config_cmd.append("-DCMAKE_C_FLAGS='{}'".format(flags))

    # Since we overwrite `CXXFLAGS` below, we must add the experimental library here for the GHA runner that builds
    # sentry-native with LLVM clang for macOS (to run ASAN on macOS) rather than the version coming with XCode.
    # TODO: remove this if the GHA runner image for macOS ever updates beyond llvm15.
    if (
        sys.platform == "darwin"
        and os.environ.get("CC", "") == "clang"
        and shutil.which("clang") == "/usr/local/opt/llvm@15/bin/clang"
    ):
        flags = (
            flags
            + " -L/usr/local/opt/llvm@15/lib/c++ -fexperimental-library -Wno-unused-command-line-argument"
        )

    config_cmd.append("-DCMAKE_CXX_FLAGS='{}'".format(flags))
