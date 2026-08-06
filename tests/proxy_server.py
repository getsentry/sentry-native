"""Simple, lightweight, dependency-free mitmdump replacement for proxy tests."""

import argparse
import base64
import select
import socket
import sys
import threading
from urllib.parse import urlsplit

BUFFER_SIZE = 64 * 1024
HEADER_LIMIT = 1024 * 1024
SOCKS_VERSION = 5


class ProxyError(Exception):
    pass


def _recv_exact(sock, size):
    data = b""
    while len(data) < size:
        chunk = sock.recv(size - len(data))
        if not chunk:
            raise ProxyError("unexpected EOF")
        data += chunk
    return data


def _recv_until(sock, marker, limit=HEADER_LIMIT):
    data = b""
    while marker not in data:
        chunk = sock.recv(BUFFER_SIZE)
        if not chunk:
            raise ProxyError("unexpected EOF")
        data += chunk
        if len(data) > limit:
            raise ProxyError("header too large")
    return data


def _parse_headers(header_lines):
    headers = []
    for line in header_lines:
        if not line:
            continue
        name, _, value = line.partition(":")
        headers.append((name.strip(), value.strip()))
    return headers


def _get_header(headers, name):
    name = name.lower()
    for key, value in headers:
        if key.lower() == name:
            return value
    return None


def _send_proxy_auth_required(client, method):
    response = (
        b"HTTP/1.1 407 Proxy Authentication Required\r\n"
        b'Proxy-Authenticate: Basic realm="sentry-native-test"\r\n'
        b"Content-Length: 0\r\n"
        b"Connection: close\r\n"
        b"\r\n"
    )
    client.sendall(response)
    print(f"{method} 407 Proxy Authentication Required", flush=True)


def _auth_matches(headers, proxy_auth):
    if not proxy_auth:
        return True
    expected = "Basic " + base64.b64encode(proxy_auth.encode("utf-8")).decode("ascii")
    return _get_header(headers, "Proxy-Authorization") == expected


def _read_http_body(client, headers, initial):
    transfer_encoding = _get_header(headers, "Transfer-Encoding")
    if transfer_encoding and "chunked" in transfer_encoding.lower():
        return _read_chunked_body(client, initial)

    content_length = _get_header(headers, "Content-Length")
    if content_length is None:
        return initial

    expected = int(content_length)
    body = initial
    while len(body) < expected:
        chunk = client.recv(min(BUFFER_SIZE, expected - len(body)))
        if not chunk:
            raise ProxyError("unexpected EOF while reading request body")
        body += chunk
    return body


def _read_chunked_body(client, initial):
    body = b""
    pending = initial
    while True:
        while b"\r\n" not in pending:
            chunk = client.recv(BUFFER_SIZE)
            if not chunk:
                raise ProxyError("unexpected EOF while reading chunk size")
            pending += chunk

        line, pending = pending.split(b"\r\n", 1)
        body += line + b"\r\n"
        size_text = line.split(b";", 1)[0]
        chunk_size = int(size_text, 16)
        if chunk_size == 0:
            while True:
                while b"\r\n" not in pending:
                    chunk = client.recv(BUFFER_SIZE)
                    if not chunk:
                        raise ProxyError("unexpected EOF while reading trailers")
                    pending += chunk

                line, pending = pending.split(b"\r\n", 1)
                body += line + b"\r\n"
                if not line:
                    return body

        need = chunk_size + 2
        while len(pending) < need:
            chunk = client.recv(BUFFER_SIZE)
            if not chunk:
                raise ProxyError("unexpected EOF while reading chunk")
            pending += chunk

        body += pending[:need]
        pending = pending[need:]


def _read_http_response(server):
    data = _recv_until(server, b"\r\n\r\n")
    header, body = data.split(b"\r\n\r\n", 1)
    lines = header.decode("iso-8859-1").split("\r\n")
    status_line = lines[0]
    headers = _parse_headers(lines[1:])

    transfer_encoding = _get_header(headers, "Transfer-Encoding")
    if transfer_encoding and "chunked" in transfer_encoding.lower():
        body = _read_chunked_body(server, body)
        return header + b"\r\n\r\n" + body, status_line

    content_length = _get_header(headers, "Content-Length")
    if content_length is not None:
        expected = int(content_length)
        while len(body) < expected:
            chunk = server.recv(min(BUFFER_SIZE, expected - len(body)))
            if not chunk:
                break
            body += chunk
    else:
        while True:
            chunk = server.recv(BUFFER_SIZE)
            if not chunk:
                break
            body += chunk

    return header + b"\r\n\r\n" + body, status_line


def _forward_http(client, proxy_auth):
    data = _recv_until(client, b"\r\n\r\n")
    header, initial_body = data.split(b"\r\n\r\n", 1)
    lines = header.decode("iso-8859-1").split("\r\n")
    method, target, version = lines[0].split(" ", 2)
    headers = _parse_headers(lines[1:])

    if not _auth_matches(headers, proxy_auth):
        _send_proxy_auth_required(client, method)
        return

    url = urlsplit(target)
    if not url.scheme or not url.hostname:
        host = _get_header(headers, "Host")
        if not host:
            raise ProxyError("missing absolute URL and Host header")
        target_host, target_port = _split_host_port(host, 80)
        path = target
    else:
        target_host = url.hostname
        target_port = url.port or 80
        path = url.path or "/"
        if url.query:
            path += "?" + url.query

    body = _read_http_body(client, headers, initial_body)
    outbound_headers = [
        (name, value)
        for name, value in headers
        if name.lower() not in {"proxy-authorization", "proxy-connection"}
    ]
    request = (
        f"{method} {path} {version}\r\n".encode("ascii")
        + b"".join(
            f"{name}: {value}\r\n".encode("iso-8859-1")
            for name, value in outbound_headers
        )
        + b"\r\n"
        + body
    )

    with socket.create_connection((target_host, target_port), timeout=10) as server:
        server.sendall(request)
        response, status_line = _read_http_response(server)
        client.sendall(response)

    status = " ".join(status_line.split(" ", 2)[1:])
    print(f"{method} {status}", flush=True)


def _split_host_port(host, default_port):
    if host.startswith("["):
        end = host.find("]")
        address = host[1:end]
        port = int(host[end + 2 :]) if host[end + 1 :].startswith(":") else default_port
        return address, port

    if ":" in host:
        address, port = host.rsplit(":", 1)
        return address, int(port)
    return host, default_port


def _send_socks_reply(client, status, bind_host="0.0.0.0", bind_port=0):
    try:
        socket.inet_pton(socket.AF_INET, bind_host)
        atyp = 1
        address = socket.inet_aton(bind_host)
    except OSError:
        atyp = 4
        address = socket.inet_pton(socket.AF_INET6, bind_host)
    client.sendall(
        bytes([SOCKS_VERSION, status, 0, atyp]) + address + bind_port.to_bytes(2, "big")
    )


def _read_socks_address(client):
    atyp = _recv_exact(client, 1)[0]
    if atyp == 1:
        host = socket.inet_ntoa(_recv_exact(client, 4))
    elif atyp == 3:
        length = _recv_exact(client, 1)[0]
        host = _recv_exact(client, length).decode("idna")
    elif atyp == 4:
        host = socket.inet_ntop(socket.AF_INET6, _recv_exact(client, 16))
    else:
        raise ProxyError(f"unsupported SOCKS address type {atyp}")
    port = int.from_bytes(_recv_exact(client, 2), "big")
    return host, port


def _forward_socks5(client):
    version, method_count = _recv_exact(client, 2)
    if version != SOCKS_VERSION:
        raise ProxyError(f"unsupported SOCKS version {version}")
    methods = _recv_exact(client, method_count)
    if 0 not in methods:
        client.sendall(bytes([SOCKS_VERSION, 0xFF]))
        return
    client.sendall(bytes([SOCKS_VERSION, 0]))

    version, command, _ = _recv_exact(client, 3)
    if version != SOCKS_VERSION or command != 1:
        _send_socks_reply(client, 7)
        return

    host, port = _read_socks_address(client)
    try:
        server = socket.create_connection((host, port), timeout=10)
    except OSError:
        _send_socks_reply(client, 5)
        return

    with server:
        bind_host, bind_port = server.getsockname()[:2]
        _send_socks_reply(client, 0, bind_host, bind_port)
        _relay_tunnel(client, server)


def _relay_tunnel(client, server):
    request_method = None
    response_status = None
    logged = False
    client_buffer = b""
    server_buffer = b""

    while True:
        readable, _, _ = select.select([client, server], [], [], 30)
        if not readable:
            return
        for source in readable:
            target = server if source is client else client
            data = source.recv(BUFFER_SIZE)
            if not data:
                return
            target.sendall(data)

            if logged:
                continue

            if source is client and request_method is None:
                client_buffer += data
                if b"\r\n" in client_buffer:
                    line = client_buffer.split(b"\r\n", 1)[0]
                    request_method = line.decode("iso-8859-1").split(" ", 1)[0]
            elif source is server and response_status is None:
                server_buffer += data
                if b"\r\n" in server_buffer:
                    line = server_buffer.split(b"\r\n", 1)[0]
                    parts = line.decode("iso-8859-1").split(" ", 2)
                    response_status = " ".join(parts[1:])

            if request_method and response_status:
                print(f"{request_method} {response_status}", flush=True)
                logged = True


def _handle_client(client, proxy_type, proxy_auth):
    with client:
        try:
            if proxy_type == "socks5-proxy":
                _forward_socks5(client)
            else:
                _forward_http(client, proxy_auth)
        except Exception as exc:
            print(f"proxy error: {exc}", file=sys.stderr, flush=True)


def serve(proxy_type, listen_host, port, proxy_auth, output):
    family = socket.AF_INET6 if ":" in listen_host else socket.AF_INET
    with socket.socket(family, socket.SOCK_STREAM) as listener:
        listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        listener.bind((listen_host, port))
        listener.listen()

        with open(output, "w", encoding="ascii") as f:
            f.write(str(listener.getsockname()[1]))

        while True:
            client, _ = listener.accept()
            thread = threading.Thread(
                target=_handle_client,
                args=(client, proxy_type, proxy_auth),
                daemon=True,
            )
            thread.start()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--type", choices=("http-proxy", "socks5-proxy"), required=True)
    parser.add_argument("--listen-host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=0)
    parser.add_argument("--proxy-auth")
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    serve(args.type, args.listen_host, args.port, args.proxy_auth, args.output)


if __name__ == "__main__":
    main()
