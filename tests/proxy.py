import contextlib
import os
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import time

import pytest

from tests.assertions import assert_no_proxy_request, wait_for, wait_for_stdout


@contextlib.contextmanager
def closed_port():
    """Bind a port and hold it open without listening.
    Connections are guaranteed to be refused, and no other process can claim it."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("127.0.0.1", 0))
        yield s.getsockname()[1]


def setup_proxy_env_vars(port):
    os.environ["http_proxy"] = f"http://127.0.0.1:{port}"
    os.environ["https_proxy"] = f"http://127.0.0.1:{port}"


def cleanup_proxy_env_vars():
    os.environ.pop("http_proxy", None)
    os.environ.pop("https_proxy", None)


@contextlib.contextmanager
def system_proxy(port):
    """Set the current user's system HTTP proxy to the test proxy and restore
    the previous settings afterwards. WinHTTP proxy discovery and NSURLSession
    read these settings, unlike the environment variables."""
    if sys.platform == "win32":
        with _wininet_proxy(f"127.0.0.1:{port}"):
            yield
    elif sys.platform == "darwin":
        with _networksetup_proxy(port):
            yield
    else:
        pytest.skip("no system proxy settings on this platform")


@contextlib.contextmanager
def _wininet_proxy(server):
    import ctypes
    from ctypes import wintypes

    INTERNET_OPTION_REFRESH = 37
    INTERNET_OPTION_SETTINGS_CHANGED = 39
    INTERNET_OPTION_PER_CONNECTION_OPTION = 75
    INTERNET_PER_CONN_FLAGS = 1
    INTERNET_PER_CONN_PROXY_SERVER = 2
    PROXY_TYPE_DIRECT = 0x1
    PROXY_TYPE_PROXY = 0x2

    class Value(ctypes.Union):
        _fields_ = [
            ("dwValue", wintypes.DWORD),
            ("pszValue", wintypes.LPWSTR),
            ("pValue", ctypes.c_void_p),
            ("ftValue", wintypes.FILETIME),
        ]

    class Option(ctypes.Structure):
        _fields_ = [("dwOption", wintypes.DWORD), ("Value", Value)]

    class OptionList(ctypes.Structure):
        _fields_ = [
            ("dwSize", wintypes.DWORD),
            ("pszConnection", wintypes.LPWSTR),
            ("dwOptionCount", wintypes.DWORD),
            ("dwOptionError", wintypes.DWORD),
            ("pOptions", ctypes.POINTER(Option)),
        ]

    wininet = ctypes.WinDLL("wininet", use_last_error=True)
    wininet.InternetQueryOptionW.argtypes = [
        ctypes.c_void_p,
        wintypes.DWORD,
        ctypes.c_void_p,
        ctypes.POINTER(wintypes.DWORD),
    ]
    wininet.InternetSetOptionW.argtypes = [
        ctypes.c_void_p,
        wintypes.DWORD,
        ctypes.c_void_p,
        wintypes.DWORD,
    ]
    kernel32 = ctypes.WinDLL("kernel32")
    kernel32.GlobalFree.argtypes = [ctypes.c_void_p]

    def make_list(flags=None, proxy_server=None):
        options = (Option * 2)()
        options[0].dwOption = INTERNET_PER_CONN_FLAGS
        options[0].Value.dwValue = flags or 0
        options[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER
        options[1].Value.pszValue = proxy_server
        option_list = OptionList(ctypes.sizeof(OptionList), None, 2, 0, options)
        return option_list, options

    def set_options(flags, proxy_server):
        option_list, _ = make_list(flags, proxy_server)
        if not wininet.InternetSetOptionW(
            None,
            INTERNET_OPTION_PER_CONNECTION_OPTION,
            ctypes.byref(option_list),
            ctypes.sizeof(option_list),
        ):
            raise ctypes.WinError(ctypes.get_last_error())
        wininet.InternetSetOptionW(None, INTERNET_OPTION_SETTINGS_CHANGED, None, 0)
        wininet.InternetSetOptionW(None, INTERNET_OPTION_REFRESH, None, 0)

    option_list, options = make_list()
    size = wintypes.DWORD(ctypes.sizeof(option_list))
    if not wininet.InternetQueryOptionW(
        None,
        INTERNET_OPTION_PER_CONNECTION_OPTION,
        ctypes.byref(option_list),
        ctypes.byref(size),
    ):
        raise ctypes.WinError(ctypes.get_last_error())
    previous_flags = options[0].Value.dwValue
    previous_server = options[1].Value.pszValue
    if options[1].Value.pValue:
        kernel32.GlobalFree(options[1].Value.pValue)

    set_options(PROXY_TYPE_DIRECT | PROXY_TYPE_PROXY, server)
    try:
        yield
    finally:
        set_options(previous_flags, previous_server)


@contextlib.contextmanager
def _networksetup_proxy(port):
    if subprocess.run(["sudo", "-n", "true"]).returncode != 0:
        pytest.skip("setting the system proxy needs sudo")

    services = [
        service
        for service in subprocess.check_output(
            ["networksetup", "-listallnetworkservices"], text=True
        ).splitlines()[1:]
        if not service.startswith("*")
    ]
    previous = {}
    for service in services:
        output = subprocess.check_output(
            ["networksetup", "-getwebproxy", service], text=True
        )
        previous[service] = dict(
            line.split(": ", 1) for line in output.splitlines() if ": " in line
        )

    try:
        for service in services:
            subprocess.check_call(
                [
                    "sudo",
                    "networksetup",
                    "-setwebproxy",
                    service,
                    "127.0.0.1",
                    str(port),
                ]
            )
        yield
    finally:
        for service, settings in previous.items():
            if settings.get("Server"):
                subprocess.check_call(
                    [
                        "sudo",
                        "networksetup",
                        "-setwebproxy",
                        service,
                        settings["Server"],
                        settings["Port"],
                    ]
                )
            if settings.get("Enabled") != "Yes":
                subprocess.check_call(
                    ["sudo", "networksetup", "-setwebproxystate", service, "off"]
                )


def _wait_for_output_file(process, output_file, timeout=10):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if process.poll() is not None:
            stdout, _ = process.communicate(timeout=1)
            raise RuntimeError(
                "test proxy exited with code {} before listening:\n{}".format(
                    process.returncode, stdout.decode("utf-8", errors="replace")
                )
            )
        try:
            port = Path(output_file).read_text(encoding="ascii")
        except OSError:
            port = ""
        if port:
            return int(port)
        time.sleep(0.05)
    raise TimeoutError(
        f"test proxy (pid {process.pid}) did not start listening within {timeout}s"
    )


def start_proxy(
    proxy_type,
    proxy_auth: str = None,
    listen_host: str = "127.0.0.1",
    retries: int = 3,
    throttle: int = 0,
):
    """Start the stdlib test proxy on a free port. Returns (process, port)."""
    proxy_server = Path(__file__).with_name("proxy_server.py")
    for attempt in range(1, retries + 1):
        output = tempfile.NamedTemporaryFile(delete=False)
        output_file = output.name
        output.close()
        try:
            os.unlink(output_file)
        except OSError:
            pass

        proxy_command = [
            sys.executable,
            "-u",
            str(proxy_server),
            "--type",
            proxy_type,
            "--listen-host",
            listen_host,
            "--port",
            "0",
            "--output",
            output_file,
        ]

        if proxy_auth:
            proxy_command += ["--proxy-auth", proxy_auth]
        if throttle:
            proxy_command += ["--throttle", str(throttle)]

        proxy_process = subprocess.Popen(
            proxy_command,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )

        try:
            port = _wait_for_output_file(proxy_process, output_file)
            return proxy_process, port
        except (TimeoutError, RuntimeError) as e:
            proxy_process.kill()
            proxy_process.wait()
            if attempt < retries:
                print(f"test proxy attempt {attempt}/{retries} failed, retrying: {e}")
                continue
            pytest.fail(str(e))
        finally:
            try:
                os.unlink(output_file)
            except OSError:
                pass

    pytest.fail("start_proxy: all retries exhausted")


def proxy_test_finally(
    expected_httpserver_logsize,
    httpserver,
    proxy_process,
    proxy_log_assert=assert_no_proxy_request,
    expected_proxy_logsize=None,
    timeout=10,
):
    if expected_proxy_logsize is None:
        expected_proxy_logsize = expected_httpserver_logsize

    if proxy_process:
        try:
            # Give the proxy some time to get a response from the mock server.
            assert wait_for(
                lambda: len(httpserver.log) >= expected_httpserver_logsize, timeout
            )

            if expected_proxy_logsize != 0:
                # request passed through successfully
                wait_for_stdout(
                    proxy_process,
                    lambda text: "POST" in text and "200 OK" in text,
                    timeout,
                )
        finally:
            proxy_process.terminate()
            proxy_process.wait(timeout=timeout)

        if expected_proxy_logsize == 0:
            # don't expect any incoming requests to make it through the proxy
            stdout_bytes, _ = proxy_process.communicate(timeout=timeout)
            stdout = stdout_bytes.decode("utf-8", errors="replace")
            proxy_log_assert(stdout)
    assert len(httpserver.log) == expected_httpserver_logsize
