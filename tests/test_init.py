"""Python equivalent of upnp/test/test_init.c"""

import ctypes
import ctypes.util
import os
import re
import pytest

UPNP_E_SUCCESS = 0


def _find_library():
    build_dir = os.environ.get("UPNP_BUILD_DIR", "")
    if build_dir:
        candidate = os.path.join(build_dir, "upnp", "libupnp.so")
        if os.path.exists(candidate):
            return candidate
    return ctypes.util.find_library("upnp")


def _parse_config():
    build_dir = os.environ.get("UPNP_BUILD_DIR", "")
    if not build_dir:
        return None
    header = os.path.join(build_dir, "upnp", "inc", "upnpconfig.h")
    if not os.path.exists(header):
        return None
    text = open(header).read()

    def int_val(name):
        m = re.search(rf"^\s*#define\s+{name}\s+(\d+)", text, re.MULTILINE)
        return int(m.group(1)) if m else None

    def str_val(name):
        m = re.search(rf'^\s*#define\s+{name}\s+"([^"]+)"', text, re.MULTILINE)
        return m.group(1) if m else None

    return {
        "UPNP_VERSION_STRING": str_val("UPNP_VERSION_STRING"),
        "UPNP_VERSION_MAJOR": int_val("UPNP_VERSION_MAJOR"),
        "UPNP_VERSION_MINOR": int_val("UPNP_VERSION_MINOR"),
        "UPNP_VERSION_PATCH": int_val("UPNP_VERSION_PATCH"),
        "UPNP_HAVE_CLIENT": int_val("UPNP_HAVE_CLIENT"),
        "UPNP_HAVE_DEVICE": int_val("UPNP_HAVE_DEVICE"),
        "UPNP_HAVE_WEBSERVER": int_val("UPNP_HAVE_WEBSERVER"),
        "UPNP_HAVE_TOOLS": int_val("UPNP_HAVE_TOOLS"),
    }


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
    handle.UpnpGetErrorMessage.restype = ctypes.c_char_p
    handle.UpnpGetErrorMessage.argtypes = [ctypes.c_int]
    return handle


def test_version_string():
    cfg = _parse_config()
    if cfg is None:
        pytest.skip("upnpconfig.h not found; set UPNP_BUILD_DIR")

    version_str = cfg["UPNP_VERSION_STRING"]
    major = cfg["UPNP_VERSION_MAJOR"]
    minor = cfg["UPNP_VERSION_MINOR"]
    patch = cfg["UPNP_VERSION_PATCH"]

    print(f'\nUPNP_VERSION_STRING = "{version_str}"')
    print(f"UPNP_VERSION_MAJOR  = {major}")
    print(f"UPNP_VERSION_MINOR  = {minor}")
    print(f"UPNP_VERSION_PATCH  = {patch}")
    print(f"UPNP_VERSION        = {(major * 100 + minor) * 100 + patch}")

    parts = version_str.split(".")
    assert len(parts) == 3, (
        f"UPNP_VERSION_STRING '{version_str}' is not in major.minor.patch format"
    )
    assert int(parts[0]) == major, f"major: string={parts[0]} macro={major}"
    assert int(parts[1]) == minor, f"minor: string={parts[1]} macro={minor}"
    assert int(parts[2]) == patch, f"patch: string={parts[2]} macro={patch}"


def test_optional_features():
    cfg = _parse_config()
    if cfg is None:
        pytest.skip("upnpconfig.h not found; set UPNP_BUILD_DIR")
    print()
    for name in (
        "UPNP_HAVE_CLIENT",
        "UPNP_HAVE_DEVICE",
        "UPNP_HAVE_WEBSERVER",
        "UPNP_HAVE_TOOLS",
    ):
        print(f"{name}\t= {'yes' if cfg.get(name) else 'no'}")


def test_init_finish(lib):
    rc = lib.UpnpInit2(None, 0)
    try:
        err = lib.UpnpGetErrorMessage(rc).decode()
        assert rc == UPNP_E_SUCCESS, f"UpnpInit2 failed: rc={rc} ({err})"

        ip = lib.UpnpGetServerIpAddress()
        port = lib.UpnpGetServerPort()
        print(f"\nUPnP initialized: ip={ip.decode() if ip else 'UNKNOWN'}, port={port}")

        assert ip is not None, "UpnpGetServerIpAddress returned NULL"
        # Without the internal web server no HTTP port is opened.
        cfg = _parse_config()
        if cfg is None or cfg["UPNP_HAVE_WEBSERVER"]:
            assert port != 0, "UpnpGetServerPort returned 0"
        else:
            assert port == 0, "UpnpGetServerPort returned a port without a web server"
    finally:
        lib.UpnpFinish()
