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
    proxy_type, proxy_auth: str = None, listen_host: str = "127.0.0.1", retries: int = 3
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
