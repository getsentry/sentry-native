import subprocess
import sys
import os
import struct

import pytest

from . import SENTRY_VERSION, lib_name
from .conditions import (
    has_breakpad,
    has_crashpad,
    has_native,
    is_aix,
    is_android,
    is_qemu,
    is_wine,
)


def _assert_symlink(path, target):
    assert path.is_symlink()
    assert os.readlink(path) == target


def _assert_shared_version(directory):
    major, minor, _ = SENTRY_VERSION.split(".")

    if sys.platform == "darwin":
        real_name = f"libsentry.{SENTRY_VERSION}.dylib"
        abi_name = f"libsentry.{major}.dylib"
        _assert_symlink(directory / "libsentry.dylib", abi_name)
        _assert_symlink(directory / abi_name, real_name)
        output = subprocess.check_output(
            ["otool", "-L", directory / real_name], text=True
        )
        assert abi_name in output
        assert f"compatibility version {major}.0.0" in output
        assert f"current version {SENTRY_VERSION}" in output
    elif sys.platform == "win32":
        dll = directory / lib_name("sentry")
        with dll.open("rb") as binary:
            binary.seek(0x3C)
            pe_offset = struct.unpack("<I", binary.read(4))[0]
            binary.seek(pe_offset + 4 + 20 + 44)
            assert struct.unpack("<HH", binary.read(4)) == (int(major), int(minor))
    elif is_wine:
        assert (directory / "libsentry.dll").is_file()
    else:
        real_name = f"libsentry.so.{SENTRY_VERSION}"
        abi_name = f"libsentry.so.{major}"
        _assert_symlink(directory / "libsentry.so", abi_name)
        _assert_symlink(directory / abi_name, real_name)
        output = subprocess.check_output(
            ["readelf", "-d", directory / real_name], text=True
        )
        assert f"Library soname: [{abi_name}]" in output


@pytest.mark.skipif(is_android or is_aix, reason="test requires a desktop build")
def test_shared_lib(cmake):
    directory = cmake(
        ["sentry"],
        {"SENTRY_BACKEND": "none", "SENTRY_TRANSPORT": "none"},
    )

    _assert_shared_version(directory)


def test_static_lib(cmake):
    tmp_path = cmake(
        ["sentry_example"],
        {
            "SENTRY_BACKEND": "none",
            "SENTRY_TRANSPORT": "none",
            "BUILD_SHARED_LIBS": "OFF",
        },
    )

    # on linux we can use `ldd` to check that we don’t link to `libsentry.so`
    if sys.platform == "linux" and not is_android and not is_qemu and not is_wine:
        output = subprocess.check_output("ldd sentry_example", cwd=tmp_path, shell=True)
        assert b"libsentry.so" not in output

    # on windows, we read the file header to check that the exe is compiled correctly
    if sys.platform == "win32" or is_wine:
        with open(os.path.join(tmp_path, "sentry_example.exe"), "rb") as binary:
            binary.seek(0x3C, 0)
            offset = int.from_bytes(binary.read(4), byteorder="little")
            binary.seek(offset, 0)
            magic = binary.read(6)
            # https://docs.microsoft.com/en-us/windows/win32/debug/pe-format#signature-image-only
            if os.environ.get("TEST_X86"):
                expected = b"PE\x00\x00\x4c\x01"  # IMAGE_FILE_MACHINE_I386
            elif os.environ.get("PROCESSOR_ARCHITECTURE") == "ARM64":
                expected = b"PE\x00\x00\x64\xaa"  # IMAGE_FILE_MACHINE_ARM64
            else:
                expected = b"PE\x00\x00\x64\x86"  # IMAGE_FILE_MACHINE_AMD64
            assert magic == expected
    # similarly, we use `file` on linux
    if sys.platform == "linux" and not is_wine:
        output = subprocess.check_output(
            "file sentry_example", cwd=tmp_path, shell=True
        )
        is_32bit = os.environ.get("TEST_X86") or os.environ.get("TEST_ARM32")
        assert (b"ELF 32-bit" if is_32bit else b"ELF 64-bit") in output


@pytest.mark.skipif(not has_crashpad, reason="test needs crashpad backend")
def test_static_crashpad(cmake):
    cmake(
        ["sentry_example"],
        {
            "SENTRY_BACKEND": "crashpad",
            "SENTRY_TRANSPORT": "none",
            "BUILD_SHARED_LIBS": "OFF",
        },
    )


@pytest.mark.skipif(not has_crashpad, reason="test needs crashpad backend")
@pytest.mark.skipif(not sys.platform == "win32", reason="test requires Windows")
def test_static_crashpad_static_runtime(cmake):
    """
    When this test fails it is most likely that you didn't reflect a target change inside the `crashpad` build in the
    top-level `crashpad` target properties (`FOLDER`, `MSVC_RUNTIME_LIBRARY`) for Windows builds.
    """
    cmake(
        ["sentry_example"],
        {
            "SENTRY_BACKEND": "crashpad",
            "SENTRY_TRANSPORT": "none",
            "BUILD_SHARED_LIBS": "OFF",
            "SENTRY_BUILD_RUNTIMESTATIC": "ON",
        },
    )


@pytest.mark.skipif(not has_breakpad, reason="test needs breakpad backend")
def test_static_breakpad(cmake):
    cmake(
        ["sentry_example"],
        {
            "SENTRY_BACKEND": "breakpad",
            "SENTRY_TRANSPORT": "none",
            "BUILD_SHARED_LIBS": "OFF",
        },
    )


@pytest.mark.skipif(not has_native, reason="test needs native backend")
def test_static_native(cmake):
    cmake(
        ["sentry_example"],
        {
            "SENTRY_BACKEND": "native",
            "SENTRY_TRANSPORT": "none",
            "BUILD_SHARED_LIBS": "OFF",
        },
    )


@pytest.mark.skipif(
    sys.platform == "win32" or is_android or is_qemu,
    reason="requires nm",
)
def test_static_mpack(cmake):
    tmp_path = cmake(
        ["sentry"],
        {
            "SENTRY_BACKEND": "none",
            "SENTRY_TRANSPORT": "none",
            "BUILD_SHARED_LIBS": "OFF",
        },
    )

    library = tmp_path / "libsentry.a"
    output = subprocess.check_output(["nm", "-g", library], text=True)
    symbols = {
        line.rsplit(maxsplit=1)[-1].removeprefix("_")
        for line in output.splitlines()
        if line.split()
    }

    unprefixed = sorted(symbol for symbol in symbols if symbol.startswith("mpack_"))
    assert not unprefixed, (
        f"{library.name} contains {len(unprefixed)} unprefixed MPack symbols: "
        f"{', '.join(unprefixed[:10])}"
    )
    assert any(
        symbol.startswith("sentry__mpack_") for symbol in symbols
    ), f"{library.name} contains no Sentry-prefixed MPack symbols"
