import contextlib
import hashlib
import json
import os
import shutil
import subprocess
import sys
import platform
import re
import time
from collections import Counter, deque
from pathlib import Path

import pytest
from filelock import FileLock

from . import adb, exe_name, lib_name
from .conditions import has_sccache
from .build_config import (
    get_android_config,
    get_platform_cmake_args,
    get_cflags,
    get_tsan_env,
)

# give up the whole run when the same build error repeats this many times
build_error_limit = int(os.environ.get("SENTRY_TEST_BUILD_ERROR_LIMIT") or 0)
build_errors = Counter()

# build errors as annotated on CI
with open(Path(__file__).parent.parent / ".github" / "problem-matchers.json") as f:
    problem_matchers = [
        [re.compile(p["regexp"]) for p in m["pattern"]]
        for m in json.load(f)["problemMatcher"]
    ]

# outlives the session fixture for the terminal summary
build_stats = dict()


def format_build_stats(verbose=False):
    rows = sorted(
        build_stats.items(),
        key=lambda row: (len(row[1]["tests"]), -row[1]["seconds"]),
    )
    lines = [f"{'tests':>5}  {'time':>7}  configuration [targets]"]
    for (source_dir, config), stats in rows:
        label = f"{config} ({source_dir})" if source_dir else config
        targets = ", ".join(sorted(stats["targets"]))
        lines.append(
            f"{len(stats['tests']):>5}  {stats['seconds']:>6.1f}s  "
            f"{label or '(default)'} [{targets}]"
        )
        if verbose or len(stats["tests"]) == 1:
            lines.extend(f"{'':16}{test}" for test in sorted(stats["tests"]))
    return "\n".join(lines) + "\n"


class CMake:
    def __init__(self, factory):
        self.runs = dict()
        self.factory = factory
        # xdist workers share builds in the session root
        self.shared = None
        if os.environ.get("PYTEST_XDIST_WORKER"):
            self.shared = factory.getbasetemp().parent / "cmake"

    def compile(self, targets, options=None, cflags=None, source_dir=None):
        # We build in tmp for all of our tests. Disable the warning MSVC generates to not clutter the build logs.
        if cflags is None:
            cflags = []
        if sys.platform == "win32":
            os.environ["IgnoreWarnIntDirInTempDetected"] = "True"

        if options is None:
            options = dict()
        key = ";".join(f"{k}={v}" for k, v in sorted(options.items()))
        if cflags:
            key += ";" + ";".join(cflags)
        key = (source_dir, key)

        start = time.monotonic()

        # cache the build configuration
        if key not in self.runs:
            cwd = self.factory.mktemp("cmake")
            with self.build_dir(key, cwd) as (build_dir, configured):
                # leave out the wait for other workers
                start = time.monotonic()
                if not configured:
                    cmake_configure(build_dir, options, cflags, source_dir)
                cmake_build(build_dir, targets, options)
            self.runs[key] = (cwd, set(targets))
        else:
            # build missing targets incrementally
            cwd, built_targets = self.runs[key]
            new_targets = [t for t in targets if t not in built_targets]
            if new_targets:
                with self.build_dir(key, cwd) as (build_dir, _):
                    start = time.monotonic()
                    cmake_build(build_dir, new_targets, options)
                built_targets.update(new_targets)

        stats = build_stats.setdefault(
            key, {"seconds": 0, "targets": set(), "tests": set()}
        )
        stats["seconds"] += time.monotonic() - start
        stats["targets"].update(targets)
        stats["tests"].add(os.environ["PYTEST_CURRENT_TEST"].rsplit(" ", 1)[0])

        build_tmp_path = cwd

        # ensure that there are no left-overs from previous runs
        shutil.rmtree(build_tmp_path / ".sentry-native", ignore_errors=True)

        # Inject a sub-path into the temporary build directory as the CWD for all tests to verify UTF-8 path handling.
        if os.environ.get("UTF8_TEST_CWD"):
            # this is Thai and translates to "this is a test directory"
            utf8_parent = self.factory.mktemp("utf8")
            utf8_subpath = utf8_parent / "นี่คือไดเร็กทอรีทดสอบ"
            utf8_subpath.symlink_to(build_tmp_path, target_is_directory=True)
            return utf8_subpath

        return build_tmp_path

    @contextlib.contextmanager
    def build_dir(self, key, cwd):
        if self.shared is None:
            yield cwd, False
            return

        build_dir = self.shared / hashlib.sha256(repr(key).encode()).hexdigest()[:16]
        self.shared.mkdir(exist_ok=True)
        with FileLock(f"{build_dir}.lock"):
            configured = build_dir.exists()
            build_dir.mkdir(exist_ok=True)
            try:
                yield build_dir, configured
            except BaseException:
                # let the next worker configure from scratch
                if not configured:
                    shutil.rmtree(build_dir, ignore_errors=True)
                raise
            # run from a copy of the outputs to keep the databases apart
            for src in build_dir.iterdir():
                dst = cwd / src.name
                if not src.is_dir() and not os.path.lexists(dst):
                    shutil.copy2(src, dst, follow_symlinks=False)

    def destroy(self):
        sourcedir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
        coveragedir = os.path.join(sourcedir, "coverage")
        shutil.rmtree(coveragedir, ignore_errors=True)
        os.mkdir(coveragedir)

        if "llvm-cov" in os.environ.get("RUN_ANALYZER", ""):

            for i, (d, _) in enumerate(self.runs.values()):
                # first merge the raw profiling runs
                files = [f for f in os.listdir(d) if f.endswith(".profraw")]
                if len(files) == 0:
                    continue
                cmd = [
                    os.environ.get("LLVM_PROFDATA", "llvm-profdata"),
                    "merge",
                    "-sparse",
                    "-o=sentry.profdata",
                    *files,
                ]
                print("{} > {}".format(d, " ".join(cmd)))
                subprocess.run(cmd, cwd=d, check=True)

                # then export lcov from the profiling data, since this needs access
                # to the object files, we need to do it per-test
                objects = [
                    exe_name("sentry_test_integration"),
                    exe_name("sentry_test_unit"),
                    lib_name("sentry"),
                    exe_name("sentry-crash"),
                    exe_name("sentry_early_init"),
                    exe_name("sentry_test_platform"),
                    exe_name("sentry_test_pkgconfig"),
                ]
                cmd = [
                    os.environ.get("LLVM_COV", "llvm-cov"),
                    "export",
                    "-format=lcov",
                    "-instr-profile=sentry.profdata",
                    "--ignore-filename-regex=(external|vendor|tests|examples)",
                    *[f"-object={o}" for o in objects if d.joinpath(o).exists()],
                ]
                lcov = os.path.join(coveragedir, f"run-{i}.lcov")
                with open(lcov, "w") as lcov_file:
                    print("{} > {} > {}".format(d, " ".join(cmd), lcov))
                    subprocess.run(cmd, stdout=lcov_file, cwd=d, check=True)

        if "kcov" in os.environ.get("RUN_ANALYZER", ""):
            coverage_dirs = [
                d
                for d in [d.joinpath("coverage") for (d, _) in self.runs.values()]
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


def cmake_configure(cwd, options, cflags=None, source_dir=None):
    if cflags is None:
        cflags = []
    __tracebackhide__ = True

    options = dict(options)
    if has_sccache:
        options.update(
            {
                "CMAKE_C_COMPILER_LAUNCHER": "sccache",
                "CMAKE_CXX_COMPILER_LAUNCHER": "sccache",
            }
        )
        if sys.platform == "win32":
            # Use /Z7 instead of /Zi to embed debug info in object files. /Zi creates
            # a shared PDB that conflicts with sccache's parallelized compilation.
            options["CMAKE_MSVC_DEBUG_INFORMATION_FORMAT"] = "Embedded"
            options["CMAKE_POLICY_DEFAULT_CMP0141"] = "NEW"
    options.update(
        {
            "CMAKE_RUNTIME_OUTPUT_DIRECTORY": cwd,
            "CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG": cwd,
            "CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE": cwd,
            # Also set library output directory so shared libraries (libsentry.so/.dylib)
            # are in the same directory as executables (sentry-crash, sentry_test_integration).
            # This is needed for the native backend daemon to find sentry-crash.
            "CMAKE_LIBRARY_OUTPUT_DIRECTORY": cwd,
            "CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG": cwd,
            "CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE": cwd,
            "CMAKE_ARCHIVE_OUTPUT_DIRECTORY": cwd,
            "CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG": cwd,
            "CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE": cwd,
        }
    )

    options.update(get_android_config())

    if source_dir is None:
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
    elif sys.platform == "win32" and os.environ.get("USE_SCCACHE"):
        # The Visual Studio generator does not support CMAKE_C_COMPILER_LAUNCHER.
        # Use Ninja instead to enable sccache.
        config_cmd.extend(["-G", "Ninja"])

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

    config_cmd.append(os.fspath(source_dir))

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


def cmake_build(cwd, targets, options):
    __tracebackhide__ = True

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
    env = dict(os.environ)
    if has_sccache:
        # Each pytest run builds in a new temp directory. Paths are normalized
        # relative to the build dir to allow sccache hits across runs.
        env.setdefault("SCCACHE_BASEDIRS", str(cwd))

    buildcmd = [*cmake, "--build", "."]
    for target in targets:
        buildcmd.extend(["--target", target])
    buildcmd.append("--parallel")
    workers = os.environ.get("PYTEST_XDIST_WORKER_COUNT")
    if workers:
        # share the CPUs between workers that build at the same time
        cpus = getattr(os, "process_cpu_count", os.cpu_count)() or 1
        buildcmd.append(str(max(1, cpus // int(workers))))
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
    errors = set()
    if build_error_limit:
        lines = deque(maxlen=max(map(len, problem_matchers)))
        with subprocess.Popen(
            buildcmd, cwd=cwd, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT
        ) as process:
            for line in process.stdout:
                sys.stdout.buffer.write(line)
                sys.stdout.buffer.flush()
                lines.append(line.decode(errors="replace").rstrip("\r\n"))
                for patterns in problem_matchers:
                    if len(lines) < len(patterns):
                        continue
                    window = list(lines)[-len(patterns) :]
                    matches = [p.search(l) for p, l in zip(patterns, window)]
                    if all(matches):
                        errors.add(tuple(g for m in matches for g in m.groups()))
        returncode = process.returncode
    else:
        returncode = subprocess.run(buildcmd, cwd=cwd, env=env).returncode
    if returncode != 0:
        build_errors.update(errors)
        if any(build_errors[e] >= build_error_limit for e in errors):
            reason = f"cmake build failed {build_error_limit} times with the same error"
            print(f"::error::{reason}", flush=True)
            pytest.exit(reason, returncode=pytest.ExitCode.TESTS_FAILED)
        raise pytest.fail.Exception("cmake build failed") from None

    # check if the DLL and EXE artifacts contain version-information
    if platform.system() == "Windows":
        from tests.win_utils import check_binary_version

        check_binary_version(Path(cwd) / "sentry.dll")
        check_binary_version(Path(cwd) / "sentry-wer.dll")
        check_binary_version(Path(cwd) / "crashpad_wer.dll")
        check_binary_version(Path(cwd) / "crashpad_handler.exe")

    # CodeChecker invocations and options are documented here:
    # https://github.com/Ericsson/codechecker/blob/master/docs/analyzer/user_guide.md
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
        adb("push", "./", "/data/local/tmp", cwd=cwd, check=True)


def configure_clang_cl(config_cmd: list[str]):
    config_cmd.append("-A x64")
    config_cmd.append("-T ClangCL")


def configure_llvm_cov(config_cmd: list[str]):
    def cmake_flags(name: str, value: str) -> str:
        if os.environ.get("VS_GENERATOR_TOOLSET") == "ClangCL":
            return f"-D{name}={value}"
        return f"-D{name}='{value}'"

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
        if os.environ.get("VS_GENERATOR_TOOLSET") == "ClangCL":
            # clang-cl source-based coverage needs the profile runtime explicitly linked.
            # The flags above enable instrumentation; the runtime library merges and
            # writes the raw profile at process exit.
            profile_lib = "clang_rt.profile-x86_64.lib"
            config_cmd.append(f"-DCMAKE_EXE_LINKER_FLAGS={profile_lib}")
            config_cmd.append(f"-DCMAKE_SHARED_LINKER_FLAGS={profile_lib}")
            config_cmd.append(f"-DCMAKE_MODULE_LINKER_FLAGS={profile_lib}")
    config_cmd.append(cmake_flags("CMAKE_C_FLAGS", flags))

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

    config_cmd.append(cmake_flags("CMAKE_CXX_FLAGS", flags))
