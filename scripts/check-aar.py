#!/usr/bin/env python3

import json
import re
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path, PurePosixPath

PROJECT_DIR = Path(__file__).resolve().parent.parent
ABIS = ("arm64-v8a", "armeabi-v7a", "x86", "x86_64")
EXPORTS = {
    "sentry": r"sentry_\w+",
    "sentry-android": r"Java_io_sentry_ndk_\w+",
}
NEEDED = {"libc.so", "libdl.so", "liblog.so", "libm.so", "libsentry.so"}
PAGE_SIZE = 0x4000


def expected_files():
    files = {
        "jni/include/sentry.h",
        "prefab/modules/sentry/include/sentry.h",
        "prefab/prefab.json",
    }
    for module in EXPORTS:
        files.add(f"prefab/modules/{module}/module.json")
        for abi in ABIS:
            libs = f"prefab/modules/{module}/libs/android.{abi}"
            files.add(f"{libs}/abi.json")
            files.add(f"{libs}/lib{module}.so")
            files.add(f"jni/{abi}/lib{module}.so")
    return files


def run(*command):
    return subprocess.run(command, check=True, capture_output=True, text=True).stdout


def declared_functions():
    header = PROJECT_DIR / "include" / "sentry.h"
    # keep the macro to find declarations in the preprocessed Android API
    output = run("cc", "-E", "-D__ANDROID__", "-DSENTRY_API=SENTRY_API", header)
    return set(re.findall(r"\bSENTRY_API\b[^;(]*?\b(sentry_\w+)\s*\(", output))


def native_methods():
    methods = set()
    sources = PROJECT_DIR / "ndk/lib/src/main/java/io/sentry/ndk"
    for path in sources.glob("*.java"):
        names = re.findall(
            r"^\s*(?:\w+ )*native [^(]*\b(\w+)\(", path.read_text(), re.M
        )
        methods |= {f"Java_io_sentry_ndk_{path.stem}_{name}" for name in names}
    return methods


def readelf(option, path):
    return run("readelf", "-W", option, path)


def exported_symbols(path):
    symbols = set()
    for line in readelf("--dyn-syms", path).splitlines():
        fields = line.split()
        if len(fields) >= 8 and fields[0][:-1].isdigit() and fields[6] != "UND":
            symbols.add(fields[7].split("@")[0])
    return symbols


def needed_libraries(path):
    return set(re.findall(r"\(NEEDED\).*\[(.+)\]", readelf("--dynamic", path)))


def load_alignments(path):
    output = readelf("--segments", path)
    return [int(a, 16) for a in re.findall(r"^\s*LOAD\s.*\s(0x\w+)$", output, re.M)]


def check_library(name, path, required):
    errors = []
    module = PurePosixPath(name).stem[len("lib") :]
    symbols = exported_symbols(path)
    missing = required[module] - symbols
    if missing:
        errors.append(f"{name}: missing exports: {sorted(missing)}")
    unexpected = {s for s in symbols if not re.fullmatch(EXPORTS[module], s)}
    if unexpected:
        errors.append(f"{name}: unexpected exports: {sorted(unexpected)}")
    unknown = needed_libraries(path) - NEEDED
    if unknown:
        errors.append(f"{name}: unexpected dependencies: {sorted(unknown)}")
    if any(alignment < PAGE_SIZE for alignment in load_alignments(path)):
        errors.append(f"{name}: LOAD segments are not 16 KB aligned")
    return errors


def main():
    errors = []
    expected = expected_files()
    required = {"sentry": declared_functions(), "sentry-android": native_methods()}
    with zipfile.ZipFile(sys.argv[1]) as aar, tempfile.TemporaryDirectory() as tmp:
        files = {
            name
            for name in aar.namelist()
            if name.startswith(("jni/", "prefab/")) and not name.endswith("/")
        }
        errors += [f"{name}: unexpected file" for name in sorted(files - expected)]
        errors += [f"{name}: missing file" for name in sorted(expected - files)]
        for name in sorted(files & expected):
            if name.endswith(".so"):
                errors += check_library(name, aar.extract(name, tmp), required)
            elif name.endswith("/abi.json"):
                stl = json.loads(aar.read(name))["stl"]
                if stl != "none":
                    errors.append(f"{name}: unexpected STL: {stl}")
            elif name == "prefab/prefab.json":
                package = json.loads(aar.read(name))["name"]
                if package != "sentry-native-ndk":
                    errors.append(f"{name}: unexpected package: {package}")
    if errors:
        sys.exit("\n".join(errors))
    print(f"{sys.argv[1]}: OK")


if __name__ == "__main__":
    main()
