import os
import sys
import subprocess
import pytest


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
        if "kcov" in os.environ.get("RUN_ANALYZER", ""):
            coverage_dirs = [
                d
                for d in [d.joinpath("coverage") for d in self.runs.values()]
                if d.exists()
            ]
            sourcedir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
            if len(coverage_dirs) > 0:
                subprocess.run(
                    [
                        "kcov",
                        "--clean",
                        "--merge",
                        os.path.join(sourcedir, "coverage"),
                        *coverage_dirs,
                    ]
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
    env = dict(os.environ)
    if os.environ.get("ERROR_ON_WARNINGS"):
        env["CFLAGS"] = env["CXXFLAGS"] = "-Werror"
    if sys.platform == "win32":
        # MP = object level parallelism, WX = warnings as errors
        cpus = os.cpu_count()
        env["CFLAGS"] = env["CXXFLAGS"] = "/WX /MP{}".format(cpus)
    if "gcc" in os.environ.get("RUN_ANALYZER", ""):
        env["CFLAGS"] = env["CXXFLAGS"] = "{} -fanalyzer".format(env["CFLAGS"])

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
