# regression: issue #250
import ctypes
import ctypes.util
import os
import socket
import time
from typing import Tuple
import pytest

UPNP_E_SUCCESS = 0


def _find_library():
    build_dir = os.environ.get("UPNP_BUILD_DIR", "")
    if build_dir:
        candidate = os.path.join(build_dir, "upnp", "libupnp.so")
        if os.path.exists(candidate):
            return candidate
    return ctypes.util.find_library("upnp")


@pytest.fixture(scope="module")
def lib():
    path = _find_library()
    if path is None:
        pytest.skip("libupnp not found; set UPNP_BUILD_DIR or install the library")
    handle = ctypes.CDLL(path)
    handle.UpnpInit2.restype = ctypes.c_int
    handle.UpnpInit2.argtypes = [ctypes.c_char_p, ctypes.c_ushort]
    handle.UpnpFinish.restype = ctypes.c_int
    handle.UpnpFinish.argtypes = []
    handle.UpnpGetServerIpAddress.restype = ctypes.c_char_p
    handle.UpnpGetServerIpAddress.argtypes = []
    handle.UpnpGetServerPort.restype = ctypes.c_ushort
    handle.UpnpGetServerPort.argtypes = []
    handle.UpnpSetMaxContentLength.restype = ctypes.c_int
    handle.UpnpSetMaxContentLength.argtypes = [ctypes.c_size_t]
    return handle


@pytest.fixture(scope="module")
def upnp(lib):
    rc = lib.UpnpInit2(None, 0)
    if rc != UPNP_E_SUCCESS:
        pytest.skip(f"UpnpInit2 failed: {rc}")
    yield lib
    lib.UpnpFinish()


def _server_addr(lib) -> Tuple[str, int]:
    ip = lib.UpnpGetServerIpAddress()
    port = lib.UpnpGetServerPort()
    return (ip.decode() if ip else "127.0.0.1", port)


def _http_status(addr: Tuple[str, int], headers: bytes, body: bytes = b"") -> str:
    with socket.create_connection(addr, timeout=5) as s:
        s.sendall(headers + body)
        resp = b""
        s.settimeout(5)
        try:
            while True:
                chunk = s.recv(4096)
                if not chunk:
                    break
                resp += chunk
                if b"\r\n\r\n" in resp:
                    break
        except socket.timeout:
            pass
    return resp.decode(errors="replace").split("\r\n")[0]


def _soap_headers(addr: Tuple[str, int], content_length: int) -> bytes:
    host, port = addr
    return (
        f"POST /upnp/control/test HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f'Content-Type: text/xml; charset="utf-8"\r\n'
        f'SOAPACTION: "urn:schemas-upnp-org:service:Test:1#Action"\r\n'
        f"Content-Length: {content_length}\r\n"
        f"\r\n"
    ).encode()


def test_oversized_soap_body_rejected_with_413(upnp):
    addr = _server_addr(upnp)
    body = b"<soap>" + b"<a/>" * 5000 + b"</soap>"  # ~20 006 bytes > 16 000
    status = _http_status(addr, _soap_headers(addr, len(body)), body)
    assert "413" in status, (
        f"Oversized SOAP body should be rejected with HTTP 413, got: {status!r}"
    )


def test_large_content_length_rejected_quickly_without_body(upnp):
    addr = _server_addr(upnp)
    large = 80 * 1024 * 1024  # 80 MB — the reporter's PoC size
    start = time.monotonic()
    # Send only headers; body is never transmitted
    status = _http_status(addr, _soap_headers(addr, large))
    elapsed = time.monotonic() - start
    assert "413" in status, (
        f"80 MB Content-Length should be rejected with HTTP 413, got: {status!r}"
    )
    assert elapsed < 2.0, (
        f"Rejection took {elapsed:.2f}s; early check should fire within 2s"
    )


def test_valid_small_soap_not_rejected(upnp):
    addr = _server_addr(upnp)
    body = (
        b'<?xml version="1.0" encoding="utf-8"?>'
        b'<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/">'
        b"<s:Body>"
        b'<u:GetVolume xmlns:u="urn:schemas-upnp-org:service:Test:1">'
        b"<InstanceID>0</InstanceID>"
        b"</u:GetVolume>"
        b"</s:Body>"
        b"</s:Envelope>"
    )
    status = _http_status(addr, _soap_headers(addr, len(body)), body)
    assert "413" not in status, (
        f"Valid small SOAP body must not be rejected with 413, got: {status!r}"
    )
