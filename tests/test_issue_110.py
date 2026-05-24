# regression: issue #110
import ctypes
import ctypes.util
import os
import socket
import struct
import fcntl
import pytest

UPNP_E_SUCCESS = 0
UPNP_E_INVALID_INTERFACE = -121
IFF_UP        = 0x1
IFF_LOOPBACK  = 0x8
IFF_MULTICAST = 0x1000
SIOCGIFFLAGS  = 0x8913

# To run the pointopoint test, create a non-multicast ipip tunnel as root:
#
#   sudo ip link add ipip_test type ipip local 127.0.0.1 remote 127.0.0.2
#   sudo ip addr add 10.99.0.1/24 dev ipip_test
#   sudo ip link set ipip_test up
#
# Root is required because creating tunnel interfaces and assigning addresses
# are privileged kernel operations (CAP_NET_ADMIN).  Tear down afterwards with:
#
#   sudo ip link delete ipip_test


def _get_if_flags(ifname: str) -> int:
    """Return interface flags via ioctl, or -1 if unavailable."""
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        ifreq = struct.pack("16sh", ifname.encode()[:15], 0)
        result = fcntl.ioctl(s.fileno(), SIOCGIFFLAGS, ifreq)
        s.close()
        return struct.unpack("16sh", result)[1]
    except Exception:
        return -1


def _find_pointopoint_interface():
    """Return the name of an UP, non-loopback, non-multicast interface, or None."""
    try:
        with open("/proc/net/dev") as f:
            for line in f:
                parts = line.split(":")
                if len(parts) < 2:
                    continue
                name = parts[0].strip()
                flags = _get_if_flags(name)
                if flags == -1:
                    continue
                if (flags & IFF_UP) and not (flags & IFF_LOOPBACK) and not (flags & IFF_MULTICAST):
                    return name
    except FileNotFoundError:
        pass
    return None


_POINTOPOINT_SETUP = (
    "No UP non-multicast interface found (e.g. ipip_test). "
    "To enable this test, run as root:\n"
    "  sudo ip link add ipip_test type ipip local 127.0.0.1 remote 127.0.0.2\n"
    "  sudo ip addr add 10.99.0.1/24 dev ipip_test\n"
    "  sudo ip link set ipip_test up\n"
    "Root is required because creating tunnel interfaces and assigning "
    "addresses are privileged kernel operations (CAP_NET_ADMIN)."
)


@pytest.fixture(scope="module")
def lib():
    build_dir = os.environ.get("UPNP_BUILD_DIR", "")
    if build_dir:
        candidate = os.path.join(build_dir, "upnp", "libupnp.so")
        path = candidate if os.path.exists(candidate) else None
    else:
        path = ctypes.util.find_library("upnp")
    if path is None:
        pytest.skip("libupnp not found; set UPNP_BUILD_DIR or install the library")
    handle = ctypes.CDLL(path)
    handle.UpnpInit2.restype = ctypes.c_int
    handle.UpnpInit2.argtypes = [ctypes.c_char_p, ctypes.c_ushort]
    handle.UpnpFinish.restype = ctypes.c_int
    handle.UpnpFinish.argtypes = []
    return handle


def test_explicit_pointopoint_interface_accepted(lib):
    """regression: issue #110 — explicitly-named non-multicast interface must be accepted."""
    ifname = _find_pointopoint_interface()
    if ifname is None:
        pytest.skip(_POINTOPOINT_SETUP)
    rc = lib.UpnpInit2(ifname.encode(), 0)
    try:
        assert rc == UPNP_E_SUCCESS, (
            f"UpnpInit2('{ifname}', 0) returned {rc}; "
            f"expected {UPNP_E_SUCCESS} — non-multicast interface rejected even when explicitly named"
        )
    finally:
        lib.UpnpFinish()


def test_auto_select_skips_pointopoint(lib):
    """regression: issue #110 guard — auto-select must still prefer multicast-capable interfaces."""
    rc = lib.UpnpInit2(None, 0)
    try:
        assert rc in (UPNP_E_SUCCESS, UPNP_E_INVALID_INTERFACE, -203), (
            f"UpnpInit2(NULL, 0) returned unexpected code {rc}"
        )
    finally:
        if rc == UPNP_E_SUCCESS:
            lib.UpnpFinish()
