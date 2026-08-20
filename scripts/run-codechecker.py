#!/usr/bin/env python3

import json
import os
import subprocess
from pathlib import Path

BACKENDS = ("none", "inproc", "breakpad", "crashpad", "native")
PROJECT_DIR = Path(__file__).resolve().parent.parent
BUILD_ROOT = PROJECT_DIR / "build" / "codechecker"


def run(command, *, check=True):
    print("+ {}".format(" ".join(map(str, command))), flush=True)
    return subprocess.run(command, cwd=PROJECT_DIR, check=check)


def main():
    run(["CodeChecker", "version"])

    compilation = []

    for backend in BACKENDS:
        build_dir = BUILD_ROOT / backend
        run(
            [
                "cmake",
                "-S",
                PROJECT_DIR,
                "-B",
                build_dir,
                "-DCMAKE_BUILD_TYPE=Debug",
                "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
                "-DSENTRY_BUILD_EXAMPLES=OFF",
                "-DSENTRY_BUILD_TESTS=OFF",
                "-DSENTRY_BACKEND={}".format(backend),
            ]
        )
        run(["cmake", "--build", build_dir, "--target", "sentry", "--parallel"])

        with (build_dir / "compile_commands.json").open() as commands:
            compilation.extend(json.load(commands))

    compilation_database = BUILD_ROOT / "compile_commands.json"
    with compilation_database.open("w") as commands:
        json.dump(compilation, commands)

    disabled_checkers = (
        "readability-magic-numbers",
        "cppcoreguidelines-avoid-magic-numbers",
        "readability-else-after-return",
        "clang-diagnostic-reserved-identifier",
        "clang-diagnostic-reserved-macro-identifier",
        "cert-err33-c",
    )
    result = run(
        [
            "CodeChecker",
            "check",
            "--jobs",
            str(os.cpu_count()),
            "--analyzers",
            "clangsa",
            "clang-tidy",
            *("--disable={}".format(checker) for checker in disabled_checkers),
            "--print-steps",
            "--ignore",
            PROJECT_DIR / ".codechecker-ignore",
            "--suppress",
            PROJECT_DIR / ".codechecker-suppress",
            "--logfile",
            compilation_database,
        ],
        check=False,
    )
    return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
