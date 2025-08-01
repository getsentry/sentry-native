import json
import os
import shutil
import subprocess
import sys
import platform
from pathlib import Path

import pytest


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

        if key not in self.runs:
            cwd = self.factory.mktemp("cmake")
            self.runs[key] = cwd
            cmake(cwd, targets, options, cflags)

        return self.runs[key]

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
    options.update(
        {
            "CMAKE_RUNTIME_OUTPUT_DIRECTORY": cwd,
            "CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG": cwd,
            "CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE": cwd,
        }
    )
    if os.environ.get("ANDROID_API") and os.environ.get("ANDROID_NDK"):
        # See: https://developer.android.com/ndk/guides/cmake
        toolchain = "{}/ndk/{}/build/cmake/android.toolchain.cmake".format(
            os.environ["ANDROID_HOME"], os.environ["ANDROID_NDK"]
        )
        options.update(
            {
                "CMAKE_TOOLCHAIN_FILE": toolchain,
                "ANDROID_ABI": os.environ.get("ANDROID_ARCH") or "x86",
                "ANDROID_NATIVE_API_LEVEL": os.environ["ANDROID_API"],
            }
        )

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
        config_cmd.append("-G Visual Studio 17 2022")
        config_cmd.append("-A x64")
        config_cmd.append("-T ClangCL")

    for key, value in options.items():
        config_cmd.append("-D{}={}".format(key, value))
    if sys.platform == "win32" and os.environ.get("TEST_X86"):
        config_cmd.append("-AWin32")
    elif sys.platform == "linux" and os.environ.get("TEST_X86"):
        config_cmd.append("-DSENTRY_BUILD_FORCE32=ON")
    if "asan" in os.environ.get("RUN_ANALYZER", ""):
        config_cmd.append("-DWITH_ASAN_OPTION=ON")
    if "tsan" in os.environ.get("RUN_ANALYZER", ""):
        module_dir = Path(__file__).resolve().parent
        tsan_options = {
            "verbosity": 2,
            "detect_deadlocks": 1,
            "second_deadlock_stack": 1,
            "suppressions": module_dir / "tsan.supp",
        }
        os.environ["TSAN_OPTIONS"] = ":".join(
            f"{key}={value}" for key, value in tsan_options.items()
        )
        config_cmd.append("-DWITH_TSAN_OPTION=ON")

    # we have to set `-Werror` for this cmake invocation only, otherwise
    # completely unrelated things will break
    if os.environ.get("ERROR_ON_WARNINGS"):
        cflags.append("-Werror")
    if sys.platform == "win32" and not os.environ.get("TEST_MINGW"):
        # MP = object level parallelism, WX = warnings as errors
        cpus = os.cpu_count()
        cflags.append("/WX /MP{}".format(cpus))
    if "gcc" in os.environ.get("RUN_ANALYZER", ""):
        cflags.append("-fanalyzer")
    if "llvm-cov" in os.environ.get("RUN_ANALYZER", ""):
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
    if "CMAKE_DEFINES" in os.environ:
        config_cmd.extend(os.environ.get("CMAKE_DEFINES").split())
    env = dict(os.environ)
    env["CFLAGS"] = env["CXXFLAGS"] = " ".join(cflags)

    config_cmd.append(source_dir)

    print("\n{} > {}".format(cwd, " ".join(config_cmd)), flush=True)
    try:
        subprocess.run(config_cmd, cwd=cwd, env=env, check=True)
    except subprocess.CalledProcessError:
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
        subprocess.run(buildcmd, cwd=cwd, check=True)
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
        child = subprocess.run(checkcmd, cwd=cwd, check=True)

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
