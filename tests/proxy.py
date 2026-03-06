import contextlib
import os
import socket
import subprocess
import time

import psutil

import pytest

from tests.assertions import assert_no_proxy_request


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


def _discover_listening_port(process, timeout=10):
    """Use psutil to discover which port the process is listening on.
    Tries per-process net_connections first, falls back to system-wide
    (filtered by PID) which works on Windows without elevated privileges."""
    deadline = time.monotonic() + timeout
    proc = psutil.Process(process.pid)
    while time.monotonic() < deadline:
        if process.poll() is not None:
            raise RuntimeError(
                f"mitmdump exited with code {process.returncode} before listening"
            )
        # Try per-process first (works on macOS/Linux without privileges)
        try:
            listeners = [
                conn
                for conn in proc.net_connections(kind="tcp")
                if conn.status == psutil.CONN_LISTEN
            ]
        except (psutil.AccessDenied, OSError):
            listeners = []

        # Fall back to system-wide, filtered by PID
        if not listeners:
            try:
                listeners = [
                    conn
                    for conn in psutil.net_connections(kind="tcp")
                    if conn.status == psutil.CONN_LISTEN and conn.pid == process.pid
                ]
            except (psutil.AccessDenied, OSError):
                listeners = []

        if listeners:
            assert (
                len(listeners) == 1
            ), f"Expected mitmdump to listen on exactly one port, got: {listeners}"
            return listeners[0].laddr.port
        time.sleep(0.2)
    raise TimeoutError(
        f"mitmdump (pid {process.pid}) did not start listening within {timeout}s"
    )


def start_mitmdump(
    proxy_type, proxy_auth: str = None, listen_host: str = "127.0.0.1", retries: int = 3
):
    """Start mitmdump on a free port. Returns (process, port).
    Retries up to `retries` times if mitmdump fails to start listening."""
    for attempt in range(1, retries + 1):
        proxy_command = [
            "mitmdump",
            "--set",
            f"listen_host={listen_host}",
            "--listen-port",
            "0",
        ]

        if proxy_type == "socks5-proxy":
            proxy_command += ["--mode", "socks5"]

        if proxy_auth:
            proxy_command += ["-v", "--proxyauth", proxy_auth]

        proxy_process = subprocess.Popen(
            proxy_command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT
        )

        try:
            port = _discover_listening_port(proxy_process)
            return proxy_process, port
        except TimeoutError as e:
            proxy_process.kill()
            proxy_process.wait()
            if attempt < retries:
                print(f"mitmdump attempt {attempt}/{retries} failed, retrying: {e}")
                continue
            else:
                pytest.fail(str(e))
        except Exception:
            proxy_process.terminate()
            proxy_process.wait()
            raise
    pytest.fail("start_mitmdump: all retries exhausted")


def proxy_test_finally(
    expected_httpserver_logsize,
    httpserver,
    proxy_process,
    proxy_log_assert=assert_no_proxy_request,
    expected_proxy_logsize=None,
):
    if expected_proxy_logsize is None:
        expected_proxy_logsize = expected_httpserver_logsize

    if proxy_process:
        # Give mitmdump some time to get a response from the mock server
        time.sleep(0.5)
        proxy_process.terminate()
        stdout_bytes, _ = proxy_process.communicate()
        stdout = stdout_bytes.decode("utf-8", errors="replace")
        if expected_proxy_logsize == 0:
            # don't expect any incoming requests to make it through the proxy
            proxy_log_assert(stdout)
        else:
            # request passed through successfully
            assert "POST" in stdout and "200 OK" in stdout
    assert len(httpserver.log) == expected_httpserver_logsize
