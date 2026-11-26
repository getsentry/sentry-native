"""
Integration tests for the native backend running inside a sandbox.

On macOS, verifies that the sandbox-safe IPC (file-backed mmap, pthread mutex,
posix_spawn) works correctly when com.apple.security.app-sandbox is enabled.

On Linux, simulates sandbox restrictions: a read-only /dev/shm in a user and
mount namespace, and a seccomp filter that blocks ptrace.
"""

import ctypes
import errno
import os
import sys
import plistlib
import shutil
import struct
import subprocess
import tempfile
import textwrap

import pytest

from . import crash_env, make_dsn, Envelope
from .assertions import (
    assert_crash_stack,
    assert_minidump,
    assert_native_crash,
    wait_for_file,
)
from .conditions import has_native, is_android, is_qemu

macos_only = pytest.mark.skipif(
    not (has_native and sys.platform == "darwin" and not is_android),
    reason="macOS App Sandbox tests require native backend on macOS",
)

linux_only = pytest.mark.skipif(
    not (has_native and sys.platform == "linux" and not is_android and not is_qemu),
    reason="Linux sandbox tests require native backend on Linux",
)


def _create_sandbox_app_bundle(tmp_path, exe_name="sentry_test_integration"):
    """
    Create a minimal .app bundle with App Sandbox entitlements around
    the already-built executable and its companion sentry-crash daemon.

    Returns the path to the executable inside the bundle.
    """
    app_dir = os.path.join(str(tmp_path), "SentryTest.app")
    contents_dir = os.path.join(app_dir, "Contents")
    macos_dir = os.path.join(contents_dir, "MacOS")
    if os.path.exists(app_dir):
        shutil.rmtree(app_dir)
    os.makedirs(macos_dir)

    # Copy executable and sentry-crash into the bundle
    src_exe = os.path.join(str(tmp_path), exe_name)
    src_daemon = os.path.join(str(tmp_path), "sentry-crash")
    dst_exe = os.path.join(macos_dir, exe_name)
    dst_daemon = os.path.join(macos_dir, "sentry-crash")

    shutil.copy2(src_exe, dst_exe)
    if os.path.exists(src_daemon):
        shutil.copy2(src_daemon, dst_daemon)

    # Copy the shared library and its versioned symlink chain into the bundle
    for src_lib in tmp_path.glob("libsentry*.dylib"):
        shutil.copy2(
            src_lib,
            os.path.join(macos_dir, src_lib.name),
            follow_symlinks=False,
        )

    # Write minimal Info.plist
    info_plist = {
        "CFBundleIdentifier": "io.sentry.native.test",
        "CFBundleName": "SentryTest",
        "CFBundleExecutable": exe_name,
        "CFBundleVersion": "1.0",
        "CFBundleShortVersionString": "1.0",
        "CFBundlePackageType": "APPL",
        "LSMinimumSystemVersion": "10.15",
    }
    with open(os.path.join(contents_dir, "Info.plist"), "wb") as f:
        plistlib.dump(info_plist, f)

    # Write entitlements with App Sandbox enabled and outgoing network
    entitlements_path = os.path.join(str(tmp_path), "sandbox.entitlements")
    entitlements = {
        "com.apple.security.app-sandbox": True,
        # Allow outgoing network connections (needed for HTTP transport)
        "com.apple.security.network.client": True,
    }
    with open(entitlements_path, "wb") as f:
        plistlib.dump(entitlements, f)

    return app_dir, dst_exe, dst_daemon, entitlements_path


def _codesign_bundle(app_dir, entitlements_path, exe_name="sentry_test_integration"):
    """
    Ad-hoc sign the .app bundle with sandbox entitlements.

    IMPORTANT: Only the main executable gets sandbox entitlements.
    Helper binaries (sentry-crash daemon, dylibs) are signed ad-hoc
    WITHOUT entitlements. If a helper binary is signed with sandbox
    entitlements but lacks a CFBundleIdentifier, macOS will abort it
    with SIGTRAP during _libsecinit_appsandbox initialization.
    """
    macos_dir = os.path.join(app_dir, "Contents", "MacOS")

    # Sign helper binaries (daemon, libraries) ad-hoc WITHOUT entitlements
    for name in os.listdir(macos_dir):
        if name == exe_name:
            continue  # Main exe gets signed with the bundle
        path = os.path.join(macos_dir, name)
        if os.path.islink(path):
            continue
        if os.access(path, os.X_OK) or name.endswith(".dylib"):
            subprocess.run(
                ["codesign", "--force", "--sign", "-", path],
                check=True,
                capture_output=True,
            )

    # Sign the bundle (which signs the main executable with entitlements)
    subprocess.run(
        [
            "codesign",
            "--force",
            "--sign",
            "-",
            "--entitlements",
            entitlements_path,
            app_dir,
        ],
        check=True,
        capture_output=True,
    )


def _run_sandboxed(app_dir, exe_name, args, env, expect_failure=False):
    """
    Run the sandboxed app bundle's executable directly.
    The sandbox is enforced by the kernel based on the code signature's
    entitlements.
    """
    exe_path = os.path.join(app_dir, "Contents", "MacOS", exe_name)

    # Set DYLD_LIBRARY_PATH so the executable can find the bundled dylib
    # inside the bundle (dyld respects this even in sandbox for ad-hoc signed)
    run_env = dict(env)
    run_env["DYLD_LIBRARY_PATH"] = os.path.join(app_dir, "Contents", "MacOS")

    result = subprocess.run(
        [exe_path] + args,
        env=run_env,
        capture_output=True,
        timeout=30,
    )

    if expect_failure:
        assert result.returncode != 0, (
            f"Expected crash but process exited cleanly.\n"
            f"stdout: {result.stdout.decode(errors='replace')}\n"
            f"stderr: {result.stderr.decode(errors='replace')}"
        )
    else:
        assert result.returncode == 0, (
            f"Process failed unexpectedly (rc={result.returncode}).\n"
            f"stdout: {result.stdout.decode(errors='replace')}\n"
            f"stderr: {result.stderr.decode(errors='replace')}"
        )

    return result


def _linux_ro_shm_cmd(cmd):
    """
    Wrap `cmd` to run in a user and mount namespace with a read-only
    /dev/shm. The crash daemon inherits the namespace.
    """
    return [
        "unshare",
        "--user",
        "--map-root-user",
        "--mount",
        "--propagation",
        "private",
        "sh",
        "-c",
        'mount -t tmpfs -o ro tmpfs /dev/shm && exec "$@"',
        "sh",
        *cmd,
    ]


def _linux_userns_available():
    """
    Probe the read-only /dev/shm sandbox, which needs unprivileged user
    namespaces.
    """
    try:
        result = subprocess.run(_linux_ro_shm_cmd(["true"]), capture_output=True)
    except FileNotFoundError:
        return False
    return result.returncode == 0


# seccomp_data.arch (AUDIT_ARCH_*) and the ptrace syscall number
_PTRACE_SYSCALLS = {
    "x86_64": (0xC000003E, 101),
    "aarch64": (0xC00000B7, 117),
}


class _SockFilter(ctypes.Structure):
    _fields_ = [
        ("code", ctypes.c_ushort),
        ("jt", ctypes.c_ubyte),
        ("jf", ctypes.c_ubyte),
        ("k", ctypes.c_uint32),
    ]


class _SockFprog(ctypes.Structure):
    _fields_ = [
        ("len", ctypes.c_ushort),
        ("filter", ctypes.POINTER(_SockFilter)),
    ]


def _block_ptrace():
    """
    Return a preexec_fn that installs a seccomp filter to make ptrace fail
    with EPERM, as Flatpak does without --devel. The crash daemon inherits
    the filter.
    """
    machine = os.uname().machine
    if machine not in _PTRACE_SYSCALLS:
        pytest.skip(f"no ptrace syscall number for {machine}")
    arch, nr = _PTRACE_SYSCALLS[machine]

    BPF_LD_W_ABS = 0x20
    BPF_JEQ_K = 0x15
    BPF_RET_K = 0x06
    SECCOMP_RET_ERRNO = 0x00050000
    SECCOMP_RET_ALLOW = 0x7FFF0000
    program = (_SockFilter * 6)(
        _SockFilter(BPF_LD_W_ABS, 0, 0, 4),  # seccomp_data.arch
        _SockFilter(BPF_JEQ_K, 0, 3, arch),
        _SockFilter(BPF_LD_W_ABS, 0, 0, 0),  # seccomp_data.nr
        _SockFilter(BPF_JEQ_K, 0, 1, nr),
        _SockFilter(BPF_RET_K, 0, 0, SECCOMP_RET_ERRNO | errno.EPERM),
        _SockFilter(BPF_RET_K, 0, 0, SECCOMP_RET_ALLOW),
    )
    fprog = _SockFprog(len(program), program)

    PR_SET_NO_NEW_PRIVS = 38
    PR_SET_SECCOMP = 22
    SECCOMP_MODE_FILTER = 2
    prctl = ctypes.CDLL(None, use_errno=True).prctl
    prctl.argtypes = [ctypes.c_int] + [ctypes.c_ulong] * 4

    # Prepared before fork, so that the child only calls prctl
    def preexec():
        if prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) or prctl(
            PR_SET_SECCOMP, SECCOMP_MODE_FILTER, ctypes.addressof(fprog), 0, 0
        ):
            raise OSError(ctypes.get_errno(), "failed to install seccomp filter")

    return preexec


def _run_linux(cmd, cwd, env, expect_failure=False, **kwargs):
    result = subprocess.run(
        cmd, cwd=cwd, env=env, capture_output=True, timeout=30, **kwargs
    )
    assert (result.returncode != 0) == expect_failure, (
        f"Unexpected exit status (rc={result.returncode}).\n"
        f"stdout: {result.stdout.decode(errors='replace')}\n"
        f"stderr: {result.stderr.decode(errors='replace')}"
    )
    return result


@macos_only
def test_sandbox_macos_init_succeeds(cmake):
    """
    Verify that sentry_init() succeeds inside App Sandbox.
    This is the core regression test: before the fix, sem_open and shm_open
    would fail with EACCES inside sandbox, causing init to fail.
    """
    tmp_path = cmake(["sentry_test_integration"], {"SENTRY_BACKEND": "native"})

    app_dir, exe, daemon, ent = _create_sandbox_app_bundle(tmp_path)
    _codesign_bundle(app_dir, ent)

    # Run with no-setup to test init/shutdown without crashing
    result = _run_sandboxed(
        app_dir,
        "sentry_test_integration",
        ["log", "no-setup"],
        env=dict(os.environ),
        expect_failure=False,
    )

    # Should not contain IPC failure messages
    stderr = result.stderr.decode(errors="replace")
    assert "failed to open shared memory" not in stderr.lower()
    assert "failed to create ipc semaphore" not in stderr.lower()
    assert "posix_spawn failed" not in stderr.lower()


@macos_only
def test_sandbox_macos_crash_capture(cmake, httpserver):
    """
    Full end-to-end: crash inside sandbox -> daemon captures minidump ->
    sends envelope to HTTP server.
    """
    tmp_path = cmake(["sentry_test_integration"], {"SENTRY_BACKEND": "native"})

    app_dir, exe, daemon, ent = _create_sandbox_app_bundle(tmp_path)
    _codesign_bundle(app_dir, ent)

    httpserver.expect_oneshot_request("/api/123456/envelope/").respond_with_data("OK")

    with httpserver.wait(timeout=15) as waiting:
        _run_sandboxed(
            app_dir,
            "sentry_test_integration",
            ["log", "stdout", "crash"],
            env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
            expect_failure=True,
        )

    assert waiting.result, "Crash envelope should be received from sandboxed app"


@macos_only
def test_sandbox_macos_minidump_generated(cmake, httpserver):
    """
    Verify that a valid minidump is generated inside sandbox.
    """
    tmp_path = cmake(["sentry_test_integration"], {"SENTRY_BACKEND": "native"})

    app_dir, exe, daemon, ent = _create_sandbox_app_bundle(tmp_path)
    _codesign_bundle(app_dir, ent)

    httpserver.expect_oneshot_request("/api/123456/envelope/").respond_with_data("OK")

    with httpserver.wait(timeout=15) as waiting:
        _run_sandboxed(
            app_dir,
            "sentry_test_integration",
            ["log", "stdout", "crash"],
            env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
            expect_failure=True,
        )

    assert waiting.result

    # Verify envelope contains minidump
    assert len(httpserver.log) >= 1
    envelope = Envelope.deserialize(httpserver.log[0][0].get_data())

    has_minidump = any(
        item.headers.get("type") == "attachment"
        and item.headers.get("attachment_type") == "event.minidump"
        for item in envelope.items
    )
    assert has_minidump, "Sandboxed crash should produce minidump"

    # Verify minidump has correct signature
    for item in envelope.items:
        if (
            item.headers.get("type") == "attachment"
            and item.headers.get("attachment_type") == "event.minidump"
        ):
            signature = struct.unpack("<I", item.payload.bytes[:4])[0]
            assert signature == 0x504D444D, "Minidump should have MDMP signature"
            break


@macos_only
def test_sandbox_macos_native_stacktrace(cmake, httpserver):
    """
    Verify native stacktrace mode works in sandbox (no minidump, just
    symbolicated frames).
    """
    tmp_path = cmake(["sentry_test_integration"], {"SENTRY_BACKEND": "native"})

    app_dir, exe, daemon, ent = _create_sandbox_app_bundle(tmp_path)
    _codesign_bundle(app_dir, ent)

    httpserver.expect_oneshot_request("/api/123456/envelope/").respond_with_data("OK")

    with httpserver.wait(timeout=15) as waiting:
        _run_sandboxed(
            app_dir,
            "sentry_test_integration",
            ["log", "stdout", "crash-mode", "native", "crash"],
            env=dict(os.environ, SENTRY_DSN=make_dsn(httpserver)),
            expect_failure=True,
        )

    assert waiting.result

    assert len(httpserver.log) >= 1
    envelope = Envelope.deserialize(httpserver.log[0][0].get_data())

    # Should have native stacktrace, not minidump
    has_minidump = any(
        item.headers.get("type") == "attachment"
        and item.headers.get("attachment_type") == "event.minidump"
        for item in envelope.items
    )
    assert not has_minidump, "Native-only mode should not include minidump"

    event = envelope.get_event()
    assert event is not None, "Should have event"
    assert "exception" in event
    exc = event["exception"]["values"][0]
    assert exc["mechanism"]["type"] == "signalhandler"
    assert "stacktrace" in exc
    assert len(exc["stacktrace"]["frames"]) > 0


@linux_only
def test_sandbox_linux_ro_shm_init_succeeds(cmake):
    """
    Verify that sentry_init() succeeds with a read-only /dev/shm, as in
    containers or snap confinement.
    """
    if not _linux_userns_available():
        pytest.skip("unprivileged user namespaces are unavailable")
    tmp_path = cmake(["sentry_test_integration"], {"SENTRY_BACKEND": "native"})
    exe = str(tmp_path / "sentry_test_integration")

    result = _run_linux(
        _linux_ro_shm_cmd([exe, "log", "no-setup"]), tmp_path, dict(os.environ)
    )

    stderr = result.stderr.decode(errors="replace")
    assert "failed to create ipc semaphore" not in stderr.lower()


@linux_only
def test_sandbox_linux_ro_shm_crash_capture(cmake, httpserver):
    """
    Full end-to-end with a read-only /dev/shm: crash -> daemon captures
    minidump -> sends envelope to HTTP server.
    """
    if not _linux_userns_available():
        pytest.skip("unprivileged user namespaces are unavailable")
    tmp_path = cmake(["sentry_test_integration"], {"SENTRY_BACKEND": "native"})
    exe = str(tmp_path / "sentry_test_integration")

    httpserver.expect_oneshot_request("/api/123456/envelope/").respond_with_data("OK")

    with httpserver.wait(timeout=15) as waiting:
        _run_linux(
            _linux_ro_shm_cmd([exe, "log", "stdout", "crash"]),
            tmp_path,
            crash_env(dict(os.environ, SENTRY_DSN=make_dsn(httpserver))),
            expect_failure=True,
        )

    assert waiting.result
    assert_minidump(Envelope.deserialize(httpserver.log[0][0].get_data()))


@linux_only
def test_sandbox_linux_no_ptrace_crash_capture(cmake, httpserver):
    """
    Crash with ptrace blocked, as in Flatpak without --devel: the daemon
    cannot attach to the crashed threads.
    """
    block_ptrace = _block_ptrace()
    tmp_path = cmake(["sentry_test_integration"], {"SENTRY_BACKEND": "native"})
    exe = str(tmp_path / "sentry_test_integration")

    httpserver.expect_oneshot_request("/api/123456/envelope/").respond_with_data("OK")

    with httpserver.wait(timeout=15) as waiting:
        _run_linux(
            [exe, "log", "stdout", "crash"],
            tmp_path,
            crash_env(dict(os.environ, SENTRY_DSN=make_dsn(httpserver))),
            expect_failure=True,
            preexec_fn=block_ptrace,
        )

    assert waiting.result
    envelope = Envelope.deserialize(httpserver.log[0][0].get_data())
    assert_native_crash(envelope)
    assert_crash_stack(envelope.get_event())
    assert_minidump(envelope)


@linux_only
def test_sandbox_linux_no_shm_leak(cmake):
    """
    Verify that a normal run leaves no named semaphore behind in /dev/shm.
    """
    tmp_path = cmake(["sentry_test_integration"], {"SENTRY_BACKEND": "native"})
    exe = str(tmp_path / "sentry_test_integration")

    with subprocess.Popen([exe, "log", "no-setup"], cwd=tmp_path) as child:
        assert child.wait(timeout=30) == 0

    assert not os.path.exists(f"/dev/shm/sem.sentry-init-{child.pid}")
