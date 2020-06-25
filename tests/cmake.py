import os
import sys
import subprocess
import pytest
import shutil


class CMake:
    def __init__(self, factory):
        self.runs = dict()
        self.factory = factory

    def compile(self, targets, options=None):
        if options is None:
            options = dict()
        key = (
            ";".join(targets),
            ";".join(f"{k}={v}" for k, v in options.items()),
        )

        if key not in self.runs:
            cwd = self.factory.mktemp("cmake")
            cmake(cwd, targets, options)
            self.runs[key] = cwd

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
                    ["kcov", "--clean", "--merge", coveragedir, *coverage_dirs,]
                )


def cmake(cwd, targets, options=None):
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
    cmake = ["cmake"]
    if "scan-build" in os.environ.get("RUN_ANALYZER", ""):
        cmake = ["scan-build", "cmake"]
    configcmd = [*cmake]
    for key, value in options.items():
        configcmd.append("-D{}={}".format(key, value))
    if sys.platform == "win32" and os.environ.get("TEST_X86"):
        configcmd.append("-AWin32")
    elif sys.platform == "linux" and os.environ.get("TEST_X86"):
        configcmd.append("-DSENTRY_BUILD_FORCE32=ON")
    if "asan" in os.environ.get("RUN_ANALYZER", ""):
        configcmd.append("-DWITH_ASAN_OPTION=ON")

    cmakelists_dir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
    configcmd.append(cmakelists_dir)

    # we have to set `-Werror` for this cmake invocation only, otherwise
    # completely unrelated things will break
    cflags = []
    if os.environ.get("ERROR_ON_WARNINGS"):
        cflags.append("-Werror")
    if sys.platform == "win32":
        # MP = object level parallelism, WX = warnings as errors
        cpus = os.cpu_count()
        cflags.append("/WX /MP{}".format(cpus))
    if "gcc" in os.environ.get("RUN_ANALYZER", ""):
        cflags.append("-fanalyzer")
    if "llvm-cov" in os.environ.get("RUN_ANALYZER", ""):
        cflags.append("-fprofile-instr-generate -fcoverage-mapping")
    env = dict(os.environ)
    env["CFLAGS"] = env["CXXFLAGS"] = " ".join(cflags)

    print("\n{} > {}".format(cwd, " ".join(configcmd)), flush=True)
    try:
        subprocess.run(configcmd, cwd=cwd, env=env, check=True)
    except subprocess.CalledProcessError:
        pytest.fail("cmake configure failed")

    buildcmd = [*cmake, "--build", ".", "--parallel"]
    for target in targets:
        buildcmd.extend(["--target", target])
    print("{} > {}".format(cwd, " ".join(buildcmd)), flush=True)
    try:
        subprocess.run(buildcmd, cwd=cwd, check=True)
    except subprocess.CalledProcessError:
        pytest.fail("cmake build failed")

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
