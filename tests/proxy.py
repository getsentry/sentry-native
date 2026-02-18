import contextlib
import re
import os
import socket
import subprocess
import time

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


def _parse_listening_port(proxy_process, timeout=10):
    """Read the 'Proxy server listening at ...' line from mitmdump stdout
    and extract the port. This consumes only the first line; subsequent
    stdout (request logs) remains available via proxy_process.stdout."""
    deadline = time.monotonic() + timeout
    buf = b""
    while time.monotonic() < deadline:
        ch = proxy_process.stdout.read(1)
        if not ch:
            break
        buf += ch
        if ch == b"\n":
            match = re.search(rb"listening at .+:(\d+)", buf)
            if match:
                return int(match.group(1))
            buf = b""
        if proxy_process.poll() is not None:
            break
    pytest.fail(
        f"mitmdump did not report a listening port within {timeout}s. Output so far: {buf!r}"
    )


def start_mitmdump(proxy_type, proxy_auth: str = None, listen_host: str = "127.0.0.1"):
    """Start mitmdump with OS-assigned port (--listen-port 0). Returns (process, port)."""
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

    # mitmdump (also written in python) often buffers output long enough so that we don't catch it
    env = {**os.environ, "PYTHONUNBUFFERED": "1"}
    proxy_process = subprocess.Popen(
        proxy_command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env
    )

    try:
        port = _parse_listening_port(proxy_process)
    except Exception:
        proxy_process.terminate()
        proxy_process.wait()
        raise
    return proxy_process, port


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
