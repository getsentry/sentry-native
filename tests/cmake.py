import os
import json
import sys
import subprocess


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

    configcmd = [*cmake]
    for key, value in options.items():
        configcmd.append("-D{}={}".format(key, value))
    if sys.platform == "win32" and os.environ.get("TEST_X86"):
        configcmd.append("-AWin32")
    elif sys.platform == "linux" and os.environ.get("TEST_X86"):
        configcmd.append("-DSENTRY_BUILD_FORCE32=ON")
    if "asan" in os.environ.get("RUN_ANALYZER", ""):
        configcmd.append("-DWITH_ASAN_OPTION=ON")

    configcmd.append(source_dir)

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

    # CodeChecker invocations and options are documented here:
    # https://github.com/Ericsson/codechecker/blob/master/docs/analyzer/user_guide.md

    buildcmd = [*cmake, "--build", ".", "--parallel"]
    for target in targets:
        buildcmd.extend(["--target", target])
    if "code-checker" in os.environ.get("RUN_ANALYZER", ""):
        buildcmd = [
            "CodeChecker",
            "log",
            "--build",
            " ".join(buildcmd),
            "--output",
            "compilation.json",
        ]
    # if "code-checker" in os.environ.get("RUN_ANALYZER", ""):
    #    buildcmd = ["CodeChecker", "check", "--ctu-all", "--build", " ".join(buildcmd)]
    print("{} > {}".format(cwd, " ".join(buildcmd)), flush=True)
    try:
        subprocess.run(buildcmd, cwd=cwd, check=True)
    except subprocess.CalledProcessError:
        pytest.fail("cmake build failed")

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
        disables = []
        for d in disable:
            disables.append("--disable")
            disables.append(d)
        checkcmd = [
            "CodeChecker",
            "check",
            "--jobs",
            str(os.cpu_count()),
            "--ctu-all",
            # TODO: we currently get >300 reports with `enable-all`
            # "--enable-all",
            # *disables,
            "--print-steps",
            "--ignore",
            os.path.join(source_dir, ".codechecker-ignore"),
            "--logfile",
            "compilation.json",
        ]
        child = subprocess.run(checkcmd, stdout=subprocess.PIPE, cwd=cwd, check=True)
        sys.stdout.buffer.write(child.stdout)
        marker = b"Total number of reports: "
        errors = child.stdout[child.stdout.rfind(marker) + len(marker) :]
        errors = int(errors[: errors.find(b"\n")])
        if errors > 0:
            pytest.fail("code-checker analysis failed")

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
