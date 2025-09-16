import gzip
import subprocess
import os
import io
import json
import sys
import urllib
import pytest
import pprint
import textwrap
import socket

sourcedir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))

# https://docs.pytest.org/en/latest/assert.html#assert-details
pytest.register_assert_rewrite("tests.assertions")
from tests.assertions import assert_no_proxy_request


def format_error_output(
    title, command, working_dir, return_code, output=None, limit_lines=50
):
    """
    Format detailed error information for failed commands.

    Args:
        title: Error title (e.g., "CMAKE CONFIGURE FAILED")
        command: Command that failed (list or string)
        working_dir: Working directory where command was run
        return_code: Return code from the failed command
        output: Output from the failed command (optional)
        limit_lines: Maximum number of lines to show from output (default: 50)

    Returns:
        Formatted error message string
    """
    # Input validation
    if limit_lines <= 0:
        limit_lines = 50

    if not output:
        output = ""

    if not title:
        title = "COMMAND FAILED"

    error_details = []
    error_details.append("=" * 60)
    error_details.append(title)
    error_details.append("=" * 60)

    if isinstance(command, list):
        command_str = " ".join(str(arg) for arg in command)
    else:
        command_str = str(command)

    error_details.append(f"Command: {command_str}")
    error_details.append(f"Working directory: {working_dir}")
    error_details.append(f"Return code: {return_code}")

    if output:
        if isinstance(output, bytes):
            output = output.decode("utf-8", errors="replace")

        output_lines = output.strip().split("\n")
        if len(output_lines) > limit_lines:
            error_details.append(f"--- OUTPUT (last {limit_lines} lines) ---")
            last_lines = output_lines[-limit_lines:]
        else:
            error_details.append("--- OUTPUT ---")
            last_lines = output_lines

        error_details.append("\n".join(last_lines))

    error_details.append("=" * 60)

    # Ensure the error message ends with a newline
    return "\n".join(error_details) + "\n"


def run_with_capture_on_failure(
    command, cwd, env=None, error_title="COMMAND FAILED", failure_exception_class=None
):
    """
    Run a subprocess command with output capture, only printing output on failure.

    Args:
        command: Command to run (list)
        cwd: Working directory
        env: Environment variables (optional)
        error_title: Title for error reporting (default: "COMMAND FAILED")
        failure_exception_class: Exception class to raise on failure (optional)

    Returns:
        subprocess.CompletedProcess result on success

    Raises:
        failure_exception_class if provided, otherwise subprocess.CalledProcessError
    """
    if env is None:
        env = os.environ

    process = subprocess.Popen(
        command,
        cwd=cwd,
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
        bufsize=1,
    )

    # Capture output without streaming
    captured_output = []
    try:
        for line in process.stdout:
            captured_output.append(line)

        return_code = process.wait()
        if return_code != 0:
            raise subprocess.CalledProcessError(return_code, command)

        # Return a successful result
        return subprocess.CompletedProcess(
            command, return_code, stdout="".join(captured_output)
        )

    except subprocess.CalledProcessError as e:
        # Enhanced error reporting with captured output
        error_message = format_error_output(
            error_title, command, cwd, e.returncode, "".join(captured_output)
        )
        print(error_message, end="", flush=True)

        if failure_exception_class:
            raise failure_exception_class("command failed") from None
        else:
            raise
    finally:
        # Ensure proper cleanup of the subprocess
        if process.poll() is None:
            # Process is still running, terminate it
            try:
                process.terminate()
                # Give the process a moment to terminate gracefully
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    # Force kill if it doesn't terminate within 5 seconds
                    process.kill()
                    process.wait(timeout=1)
            except (OSError, ValueError):
                # Process might already be terminated or invalid
                pass


def make_dsn(httpserver, auth="uiaeosnrtdy", id=123456, proxy_host=False):
    url = urllib.parse.urlsplit(httpserver.url_for("/{}".format(id)))
    # We explicitly use `127.0.0.1` here, because on Windows, `localhost` will
    # first try `::1` (the ipv6 loopback), retry a couple times and give up
    # after a timeout of 2 seconds, falling back to the ipv4 loopback instead.
    host = url.netloc.replace("localhost", "127.0.0.1")
    if proxy_host:
        # To avoid bypassing the proxy for requests to localhost, we need to add this mapping
        # to the hosts file & make the DSN using this alternate hostname
        # see https://learn.microsoft.com/en-us/windows/win32/wininet/enabling-internet-functionality#listing-the-proxy-bypass
        host = host.replace("127.0.0.1", "sentry.native.test")
        _check_sentry_native_resolves_to_localhost()

    return urllib.parse.urlunsplit(
        (
            url.scheme,
            "{}@{}".format(auth, host),
            url.path,
            url.query,
            url.fragment,
        )
    )


def _check_sentry_native_resolves_to_localhost():
    try:
        resolved_ip = socket.gethostbyname("sentry.native.test")
        assert resolved_ip == "127.0.0.1"
    except socket.gaierror:
        pytest.skip("sentry.native.test does not resolve to localhost")


def run(cwd, exe, args, env=dict(os.environ), **kwargs):
    __tracebackhide__ = True
    if os.environ.get("ANDROID_API"):
        # older android emulators do not correctly pass down the returncode
        # so we basically echo the return code, and parse it manually
        capture_output = kwargs.get("stdout") != subprocess.PIPE

        if capture_output:
            # Capture output for potential display on failure
            kwargs["stdout"] = subprocess.PIPE
            kwargs["stderr"] = subprocess.STDOUT

        child = subprocess.run(
            [
                "{}/platform-tools/adb".format(os.environ["ANDROID_HOME"]),
                "shell",
                # Android by default only searches for libraries in system
                # directories and the app directory, and only supports RUNPATH
                # since API-24.
                # Since we are no "app" in that sense, we can use
                # `LD_LIBRARY_PATH` to force the android dynamic loader to
                # load `libsentry.so` from the correct library.
                # See https://android.googlesource.com/platform/bionic/+/master/android-changes-for-ndk-developers.md#dt_runpath-support-available-in-api-level-24
                "cd /data/local/tmp && LD_LIBRARY_PATH=. ./{} {}; echo -n ret:$?".format(
                    exe, " ".join(args)
                ),
            ],
            **kwargs,
        )
        stdout = child.stdout
        child.returncode = int(stdout[stdout.rfind(b"ret:") :][4:])
        child.stdout = stdout[: stdout.rfind(b"ret:")]

        # Only write output to stdout if not capturing or on success
        if not capture_output or child.returncode == 0:
            if kwargs.get("stdout") != subprocess.PIPE:
                sys.stdout.buffer.write(child.stdout)
        elif capture_output and child.returncode != 0:
            # Enhanced error reporting for Android test execution failures
            command = f"{exe} {' '.join(args)}"
            error_message = format_error_output(
                "ANDROID TEST EXECUTION FAILED",
                command,
                "/data/local/tmp",
                child.returncode,
                child.stdout,
            )
            print(error_message, end="", flush=True)

        if kwargs.get("check") and child.returncode:
            raise subprocess.CalledProcessError(
                child.returncode, child.args, output=child.stdout, stderr=child.stderr
            )
        return child

    cmd = [
        "./{}".format(exe) if sys.platform != "win32" else "{}\\{}.exe".format(cwd, exe)
    ]
    if "asan" in os.environ.get("RUN_ANALYZER", ""):
        env["ASAN_OPTIONS"] = "detect_leaks=1:detect_invalid_join=0"
        env["LSAN_OPTIONS"] = "suppressions={}".format(
            os.path.join(sourcedir, "tests", "leaks.txt")
        )
    if "llvm-cov" in os.environ.get("RUN_ANALYZER", ""):
        # continuous mode is only supported on mac right now
        continuous = "%c" if sys.platform == "darwin" else ""
        env["LLVM_PROFILE_FILE"] = f"coverage-%p{continuous}.profraw"
    if "kcov" in os.environ.get("RUN_ANALYZER", ""):
        coverage_dir = os.path.join(cwd, "coverage")
        cmd = [
            "kcov",
            "--include-path={}".format(os.path.join(sourcedir, "src")),
            coverage_dir,
            *cmd,
        ]
    if "valgrind" in os.environ.get("RUN_ANALYZER", ""):
        cmd = [
            "valgrind",
            "--suppressions={}".format(
                os.path.join(sourcedir, "tests", "valgrind.txt")
            ),
            "--error-exitcode=33",
            "--leak-check=yes",
            *cmd,
        ]

    # Capture output unless explicitly requested to pipe to caller or stream to stdout
    should_capture = kwargs.get("stdout") != subprocess.PIPE and "stdout" not in kwargs

    if should_capture:
        # Capture both stdout and stderr for potential display on failure
        kwargs_with_capture = kwargs.copy()
        kwargs_with_capture["stdout"] = subprocess.PIPE
        kwargs_with_capture["stderr"] = subprocess.STDOUT
        kwargs_with_capture["universal_newlines"] = True

        try:
            result = subprocess.run(
                [*cmd, *args], cwd=cwd, env=env, **kwargs_with_capture
            )
            if result.returncode != 0 and kwargs.get("check"):
                # Enhanced error reporting for test execution failures
                command = cmd + args
                error_message = format_error_output(
                    "TEST EXECUTION FAILED",
                    command,
                    cwd,
                    result.returncode,
                    result.stdout,
                )
                print(error_message, end="", flush=True)

                raise subprocess.CalledProcessError(result.returncode, result.args)
            return result
        except subprocess.CalledProcessError:
            raise pytest.fail.Exception(
                "running command failed: {cmd} {args}".format(
                    cmd=" ".join(cmd), args=" ".join(args)
                )
            ) from None
    else:
        # Use original behavior when stdout is explicitly handled by caller
        try:
            return subprocess.run([*cmd, *args], cwd=cwd, env=env, **kwargs)
        except subprocess.CalledProcessError:
            raise pytest.fail.Exception(
                "running command failed: {cmd} {args}".format(
                    cmd=" ".join(cmd), args=" ".join(args)
                )
            ) from None


def check_output(*args, **kwargs):
    stdout = run(*args, check=True, stdout=subprocess.PIPE, **kwargs).stdout
    # capturing stdout on windows actually encodes "\n" as "\r\n", which we
    # revert, because it messes with envelope decoding
    stdout = stdout.replace(b"\r\n", b"\n")
    return stdout


# Adapted from: https://raw.githubusercontent.com/getsentry/sentry-python/276acae955ee13f7ac3a7728003626eff6d943a8/sentry_sdk/envelope.py


class Envelope(object):
    def __init__(
        self,
        headers=None,  # type: Optional[Dict[str, str]]
        items=None,  # type: Optional[List[Item]]
    ):
        # type: (...) -> None
        if headers is not None:
            headers = dict(headers)
        self.headers = headers or {}
        if items is None:
            items = []
        else:
            items = list(items)
        self.items = items

    def get_event(self):
        # type: (...) -> Optional[Event]
        for item in self.items:
            event = item.get_event()
            if event is not None:
                return event
        return None

    def __iter__(self):
        # type: (...) -> Iterator[Item]
        return iter(self.items)

    @classmethod
    def deserialize_from(
        cls, f  # type: Any
    ):
        # type: (...) -> Envelope
        headers = json.loads(f.readline())
        items = []
        while 1:
            item = Item.deserialize_from(f)
            if item is None:
                break
            items.append(item)
        return cls(headers=headers, items=items)

    @classmethod
    def deserialize(
        cls, data  # type: bytes
    ):
        # type: (...) -> Envelope

        # check if the data is gzip encoded and extract it before deserialization.
        # 0x1f8b: gzip-magic, 0x08: `DEFLATE` compression method.
        if data[:3] == b"\x1f\x8b\x08":
            with gzip.open(io.BytesIO(data), "rb") as output:
                return cls.deserialize_from(output)

        return cls.deserialize_from(io.BytesIO(data))

    def print_verbose(self, indent=0):
        """Pretty prints the envelope."""
        print(" " * indent + "Envelope:")
        indent += 2
        print(" " * indent + "Headers:")
        indent += 2
        print(textwrap.indent(pprint.pformat(self.headers), " " * indent))
        indent -= 2
        print(" " * indent + "Items:")
        indent += 2
        for item in self.items:
            item.print_verbose(indent)

    def __repr__(self):
        # type: (...) -> str
        return "<Envelope headers=%r items=%r>" % (self.headers, self.items)


class PayloadRef(object):
    def __init__(
        self,
        bytes=None,  # type: Optional[bytes]
        json=None,  # type: Optional[Any]
    ):
        # type: (...) -> None
        self.json = json
        self.bytes = bytes

    def print_verbose(self, indent=0):
        """Pretty prints the item."""
        print(" " * indent + "Payload:")
        indent += 2
        if self.bytes:
            payload = self.bytes.encode("utf-8", errors="replace")
        if self.json:
            payload = pprint.pformat(self.json)
        try:
            print(textwrap.indent(payload, " " * indent))
        except UnicodeEncodeError:
            # Windows CI appears fickle, and we put emojis in the json
            payload = payload.encode("ascii", errors="replace").decode()
            print(textwrap.indent(payload, " " * indent))

    def __repr__(self):
        # type: (...) -> str
        return "<Payload bytes=%r json=%r>" % (self.bytes, self.json)


class Item(object):
    def __init__(
        self,
        payload,  # type: Union[bytes, text_type, PayloadRef]
        headers=None,  # type: Optional[Dict[str, str]]
    ):
        if headers is not None:
            headers = dict(headers)
        elif headers is None:
            headers = {}
        self.headers = headers
        if isinstance(payload, bytes):
            payload = PayloadRef(bytes=payload)
        else:
            payload = payload

        self.payload = payload

    def get_event(self):
        # type: (...) -> Optional[Event]
        # Transactions are events with a few extra fields, so return them as well.
        if (
            self.headers.get("type") in ["event", "transaction"]
            and self.payload.json is not None
        ):
            return self.payload.json
        return None

    @classmethod
    def deserialize_from(
        cls, f  # type: Any
    ):
        # type: (...) -> Optional[Item]
        line = f.readline().rstrip()
        if not line:
            return None
        headers = json.loads(line)
        length = headers["length"]
        payload = f.read(length)
        if headers.get("type") in [
            "event",
            "feedback",
            "session",
            "transaction",
            "user_report",
        ]:
            rv = cls(headers=headers, payload=PayloadRef(json=json.loads(payload)))
        else:
            rv = cls(headers=headers, payload=payload)
        f.readline()
        return rv

    @classmethod
    def deserialize(
        cls, bytes  # type: bytes
    ):
        # type: (...) -> Optional[Item]
        return cls.deserialize_from(io.BytesIO(bytes))

    def print_verbose(self, indent=0):
        """Pretty prints the item."""
        item_type = self.headers.get("type", "?").capitalize()
        print(" " * indent + f"{item_type}:")
        indent += 2
        print(" " * indent + "Headers:")
        indent += 2
        headers = pprint.pformat(self.headers)
        print(textwrap.indent(headers, " " * indent))
        indent -= 2
        self.payload.print_verbose(indent)

    def __repr__(self):
        # type: (...) -> str
        return "<Item headers=%r payload=%r>" % (
            self.headers,
            self.payload,
        )
