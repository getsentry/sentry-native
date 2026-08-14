import base64
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import queue
import socket
import subprocess
import sys
import threading
import time

import pytest


class _RequestHandler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def do_GET(self):
        self._respond(b"")

    def do_POST(self):
        body = self.rfile.read(int(self.headers.get("Content-Length", "0")))
        self._respond(body)

    def _respond(self, body):
        self.server.requests.put(
            (self.command, self.path, dict(self.headers.items()), body)
        )
        self.send_response(200)
        self.send_header("Content-Length", "2")
        self.send_header("Connection", "close")
        self.end_headers()
        self.wfile.write(b"OK")

    def log_message(self, format, *args):
        pass


@pytest.fixture
def target_server():
    server = ThreadingHTTPServer(("127.0.0.1", 0), _RequestHandler)
    server.requests = queue.Queue()
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()

    yield server

    server.shutdown()
    server.server_close()
    thread.join(timeout=5)


@pytest.fixture
def start_proxy(tmp_path):
    processes = []
    proxy_server = Path(__file__).with_name("proxy_server.py")

    def start(proxy_type, proxy_auth=None):
        output = tmp_path / "proxy-{}.port".format(len(processes))
        command = [
            sys.executable,
            "-u",
            str(proxy_server),
            "--type",
            proxy_type,
            "--listen-host",
            "127.0.0.1",
            "--port",
            "0",
            "--output",
            str(output),
        ]
        if proxy_auth:
            command += ["--proxy-auth", proxy_auth]

        process = subprocess.Popen(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        processes.append(process)

        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            if process.poll() is not None:
                stdout, _ = process.communicate(timeout=1)
                pytest.fail(
                    "test proxy exited before listening:\n{}".format(
                        stdout.decode("utf-8", errors="replace")
                    )
                )
            try:
                port = output.read_text(encoding="ascii")
            except OSError:
                port = ""
            if port:
                return int(port)
            time.sleep(0.01)

        pytest.fail("test proxy did not start listening")

    yield start

    for process in processes:
        if process.poll() is None:
            process.terminate()
        try:
            process.communicate(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.communicate(timeout=5)


def _exchange(port, request):
    with socket.create_connection(("127.0.0.1", port), timeout=5) as client:
        client.sendall(request)
        response = b""
        while True:
            chunk = client.recv(64 * 1024)
            if not chunk:
                return response
            response += chunk


def _proxy_request(target_port, proxy_auth=None):
    headers = [
        "POST http://127.0.0.1:{}/envelope?test=1 HTTP/1.1".format(target_port),
        "Host: 127.0.0.1:{}".format(target_port),
        "Content-Length: 4",
        "Connection: close",
    ]
    if proxy_auth:
        encoded = base64.b64encode(proxy_auth.encode("utf-8")).decode("ascii")
        headers.append("Proxy-Authorization: Basic {}".format(encoded))
    return ("\r\n".join(headers) + "\r\n\r\ndata").encode("ascii")


def _recv_exact(sock, size):
    data = b""
    while len(data) < size:
        chunk = sock.recv(size - len(data))
        assert chunk
        data += chunk
    return data


def test_http_forwarding(start_proxy, target_server):
    port = start_proxy("http-proxy")
    target_port = target_server.server_address[1]

    response = _exchange(port, _proxy_request(target_port))

    assert response.startswith(b"HTTP/1.1 200")
    assert response.endswith(b"OK")
    method, path, _, body = target_server.requests.get(timeout=5)
    assert (method, path, body) == ("POST", "/envelope?test=1", b"data")


def test_proxy_auth_rejection(start_proxy, target_server):
    port = start_proxy("http-proxy", "user:password")
    target_port = target_server.server_address[1]

    response = _exchange(port, _proxy_request(target_port, "wrong:password"))

    assert response.startswith(b"HTTP/1.1 407")
    assert target_server.requests.empty()


def test_proxy_auth_forwarding(start_proxy, target_server):
    port = start_proxy("http-proxy", "user:password")
    target_port = target_server.server_address[1]

    response = _exchange(port, _proxy_request(target_port, "user:password"))

    assert response.startswith(b"HTTP/1.1 200")
    assert response.endswith(b"OK")
    _, _, headers, body = target_server.requests.get(timeout=5)
    assert "Proxy-Authorization" not in headers
    assert body == b"data"


def test_socks5_tunneling(start_proxy, target_server):
    port = start_proxy("socks5-proxy")
    target_port = target_server.server_address[1]

    with socket.create_connection(("127.0.0.1", port), timeout=5) as client:
        client.sendall(b"\x05\x01\x00")
        assert _recv_exact(client, 2) == b"\x05\x00"

        client.sendall(
            b"\x05\x01\x00\x01"
            + socket.inet_aton("127.0.0.1")
            + target_port.to_bytes(2, "big")
        )
        assert _recv_exact(client, 2) == b"\x05\x00"
        _recv_exact(client, 8)

        request = (
            "GET /through-socks HTTP/1.1\r\n"
            "Host: 127.0.0.1:{}\r\n"
            "Connection: close\r\n"
            "\r\n"
        ).format(target_port)
        client.sendall(request.encode("ascii"))

        response = b""
        while True:
            chunk = client.recv(64 * 1024)
            if not chunk:
                break
            response += chunk

    assert response.startswith(b"HTTP/1.1 200")
    assert response.endswith(b"OK")
    method, path, _, body = target_server.requests.get(timeout=5)
    assert (method, path, body) == ("GET", "/through-socks", b"")
