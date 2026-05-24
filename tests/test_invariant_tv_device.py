import pytest
import re
import struct
import socket
import urllib.parse
from unittest.mock import MagicMock, patch


# Simulated UPnP device endpoint handler that mimics tv_device.c behavior
class TVDeviceEndpoint:
    """Simulates the UPnP TV device endpoint with authentication checks."""
    
    MAX_BUFFER_SIZE = 256  # Fixed buffer size as in tv_device.c
    
    def __init__(self):
        self.valid_tokens = {"valid_token_abc123"}
        self.valid_session_ids = {"session_xyz789"}
    
    def _validate_auth(self, headers: dict) -> bool:
        """Validate authentication credentials from headers."""
        auth_header = headers.get("Authorization", "")
        token = headers.get("X-Auth-Token", "")
        session_id = headers.get("X-Session-ID", "")
        
        if not auth_header and not token and not session_id:
            return False
        
        # Check Bearer token
        if auth_header.startswith("Bearer "):
            bearer_token = auth_header[7:]
            if bearer_token in self.valid_tokens:
                return True
        
        # Check X-Auth-Token
        if token in self.valid_tokens:
            return True
        
        # Check session ID
        if session_id in self.valid_session_ids:
            return True
        
        return False
    
    def _validate_upnp_string(self, value: str, field_name: str) -> tuple:
        """Validate UPnP-derived strings to prevent buffer overflow."""
        if value is None:
            return False, 400
        
        # Simulate bounds checking that strcpy lacks
        if len(value.encode('utf-8')) >= self.MAX_BUFFER_SIZE:
            return False, 400
        
        # Check for null bytes (common in overflow attempts)
        if '\x00' in value:
            return False, 400
            
        return True, 200
    
    def handle_request(self, method: str, path: str, headers: dict, body: dict = None) -> dict:
        """Handle incoming request and return response dict with status code."""
        
        # Authentication check MUST happen before any processing
        if not self._validate_auth(headers):
            return {"status": 401, "body": "Unauthorized"}
        
        # Process UPnP fields only after authentication
        if body:
            for field in ["UDN", "ServiceId", "ServiceType"]:
                if field in body:
                    valid, code = self._validate_upnp_string(body[field], field)
                    if not valid:
                        return {"status": code, "body": f"Invalid {field}"}
        
        return {"status": 200, "body": "OK"}


# Test payloads representing various attack scenarios
UNAUTHENTICATED_PAYLOADS = [
    # Missing authentication entirely
    pytest.param(
        {"headers": {}, "body": {"UDN": "normal_udn", "ServiceId": "sid", "ServiceType": "stype"}},
        id="no_auth_headers"
    ),
    # Empty Authorization header
    pytest.param(
        {"headers": {"Authorization": ""}, "body": {"UDN": "normal_udn"}},
        id="empty_auth_header"
    ),
    # Malformed Bearer token
    pytest.param(
        {"headers": {"Authorization": "Bearer "}, "body": {"UDN": "normal_udn"}},
        id="bearer_empty_token"
    ),
    # Invalid Bearer token
    pytest.param(
        {"headers": {"Authorization": "Bearer invalid_token_xyz"}, "body": {}},
        id="bearer_invalid_token"
    ),
    # Expired/fake token
    pytest.param(
        {"headers": {"Authorization": "Bearer expired_token_000"}, "body": {}},
        id="bearer_expired_token"
    ),
    # SQL injection in token
    pytest.param(
        {"headers": {"Authorization": "Bearer ' OR '1'='1"}, "body": {}},
        id="sql_injection_token"
    ),
    # Buffer overflow attempt in Authorization header
    pytest.param(
        {"headers": {"Authorization": "Bearer " + "A" * 1024}, "body": {}},
        id="oversized_bearer_token"
    ),
    # Null byte injection in token
    pytest.param(
        {"headers": {"Authorization": "Bearer valid_token\x00injected"}, "body": {}},
        id="null_byte_in_token"
    ),
    # Wrong auth scheme
    pytest.param(
        {"headers": {"Authorization": "Basic dXNlcjpwYXNz"}, "body": {}},
        id="wrong_auth_scheme_basic"
    ),
    # Digest auth (wrong scheme)
    pytest.param(
        {"headers": {"Authorization": "Digest username=admin"}, "body": {}},
        id="wrong_auth_scheme_digest"
    ),
    # UPnP buffer overflow in UDN with no auth
    pytest.param(
        {"headers": {}, "body": {"UDN": "A" * 512, "ServiceId": "sid", "ServiceType": "stype"}},
        id="upnp_udn_overflow_no_auth"
    ),
    # UPnP buffer overflow in ServiceId with no auth
    pytest.param(
        {"headers": {}, "body": {"UDN": "udn", "ServiceId": "B" * 512, "ServiceType": "stype"}},
        id="upnp_serviceid_overflow_no_auth"
    ),
    # UPnP buffer overflow in ServiceType with no auth
    pytest.param(
        {"headers": {}, "body": {"UDN": "udn", "ServiceId": "sid", "ServiceType": "C" * 512}},
        id="upnp_servicetype_overflow_no_auth"
    ),
    # Crafted UPnP XML injection with no auth
    pytest.param(
        {"headers": {}, "body": {"UDN": "<UDN>malicious</UDN>", "ServiceId": "sid"}},
        id="xml_injection_no_auth"
    ),
    # Format string attack in UDN with no auth
    pytest.param(
        {"headers": {}, "body": {"UDN": "%s%s%s%s%s%n", "ServiceId": "sid"}},
        id="format_string_udn_no_auth"
    ),
    # Malformed X-Auth-Token
    pytest.param(
        {"headers": {"X-Auth-Token": "malformed!!@#$%"}, "body": {}},
        id="malformed_x_auth_token"
    ),
    # Oversized X-Auth-Token
    pytest.param(
        {"headers": {"X-Auth-Token": "X" * 2048}, "body": {}},
        id="oversized_x_auth_token"
    ),
    # Invalid session ID
    pytest.param(
        {"headers": {"X-Session-ID": "fake_session_000"}, "body": {}},
        id="invalid_session_id"
    ),
    # Unicode bypass attempt in token
    pytest.param(
        {"headers": {"Authorization": "Bearer \u202evalid_token_abc123"}, "body": {}},
        id="unicode_rtlo_token"
    ),
    # URL-encoded token bypass
    pytest.param(
        {"headers": {"Authorization": "Bearer %76alid_token_abc123"}, "body": {}},
        id="url_encoded_token"
    ),
    # Case manipulation bypass
    pytest.param(
        {"headers": {"Authorization": "BEARER valid_token_abc123"}, "body": {}},
        id="case_manipulation_bearer"
    ),
    # Multiple auth headers (only first should count, both invalid)
    pytest.param(
        {"headers": {"Authorization": "Bearer fake1", "X-Auth-Token": "fake2"}, "body": {}},
        id="multiple_invalid_auth_headers"
    ),
    # Whitespace-only token
    pytest.param(
        {"headers": {"Authorization": "Bearer    "}, "body": {}},
        id="whitespace_only_token"
    ),
    # Token with newline injection
    pytest.param(
        {"headers": {"Authorization": "Bearer valid\nX-Injected: header"}, "body": {}},
        id="newline_injection_token"
    ),
    # CRLF injection in token
    pytest.param(
        {"headers": {"Authorization": "Bearer valid\r\nX-Injected: header"}, "body": {}},
        id="crlf_injection_token"
    ),
    # Binary data in token
    pytest.param(
        {"headers": {"Authorization": "Bearer " + struct.pack(">I", 0xDEADBEEF).decode('latin-1')}, "body": {}},
        id="binary_data_token"
    ),
    # UPnP SOAP action with oversized strings and no auth
    pytest.param(
        {
            "headers": {},
            "body": {
                "UDN": "uuid:" + "F" * 300,
                "ServiceId": "urn:upnp-org:serviceId:" + "G" * 300,
                "ServiceType": "urn:schemas-upnp-org:service:" + "H" * 300
            }
        },
        id="upnp_soap_oversized_all_fields_no_auth"
    ),
    # Null session ID
    pytest.param(
        {"headers": {"X-Session-ID": ""}, "body": {}},
        id="empty_session_id"
    ),
    # Token that looks valid but isn't
    pytest.param(
        {"headers": {"Authorization": "Bearer valid_token_abc124"}, "body": {}},  # off by one char
        id="near_valid_token"
    ),
]


@pytest.fixture
def tv_device():
    """Fixture providing a TVDeviceEndpoint instance."""
    return TVDeviceEndpoint()


@pytest.mark.parametrize("payload", UNAUTHENTICATED_PAYLOADS)
def test_protected_endpoints_reject_unauthenticated_requests(payload, tv_device):
    """
    Invariant: Protected UPnP TV device endpoints MUST reject all requests
    that do not carry valid authentication credentials with HTTP 401 or 403.
    
    This guards against CWE-287 (Improper Authentication) where unauthenticated
    UPnP requests could reach buffer-unsafe code paths (strcpy with UDN,
    ServiceId, ServiceType) allowing buffer overflow attacks from local network.
    
    Authentication MUST be enforced BEFORE any UPnP string processing occurs.
    """
    headers = payload["headers"]
    body = payload.get("body", {})
    
    response = tv_device.handle_request(
        method="POST",
        path="/upnp/control",
        headers=headers,
        body=body
    )
    
    status_code = response["status"]
    
    # The endpoint MUST reject unauthenticated requests with 401 or 403
    assert status_code in (401, 403), (
        f"Security invariant violated: endpoint returned HTTP {status_code} "
        f"instead of 401/403 for unauthenticated request.\n"
        f"Headers: {headers}\n"
        f"Body keys: {list(body.keys())}\n"
        f"This means unauthenticated UPnP strings could reach unsafe strcpy calls, "
        f"enabling buffer overflow attacks (CWE-287 + CWE-120)."
    )
    
    # Additionally verify the response body does NOT contain sensitive data
    response_body = response.get("body", "")
    assert "OK" not in str(response_body), (
        f"Security invariant violated: unauthenticated request received success response body. "
        f"Status: {status_code}, Body: {response_body}"
    )


@pytest.mark.parametrize("payload", UNAUTHENTICATED_PAYLOADS)
def test_auth_check_precedes_upnp_processing(payload, tv_device):
    """
    Invariant: Authentication validation MUST occur before any UPnP-derived
    string processing. Unauthenticated requests must never reach strcpy calls
    that copy UDN, ServiceId, or ServiceType into fixed-size buffers.
    
    This ensures that even if oversized/malicious UPnP strings are sent,
    they are rejected at the authentication layer before reaching vulnerable
    buffer operations.
    """
    headers = payload["headers"]
    body = payload.get("body", {})
    
    # Track whether auth was checked before string processing
    auth_checked_first = []
    original_validate_auth = tv_device._validate_auth
    original_validate_string = tv_device._validate_upnp_string
    
    call_order = []
    
    def tracking_validate_auth(h):
        call_order.append("auth")
        return original_validate_auth(h)
    
    def tracking_validate_string(v, f):
        call_order.append("string_processing")
        return original_validate_string(v, f)
    
    tv_device._validate_auth = tracking_validate_auth
    tv_device._validate_upnp_string = tracking_validate_string
    
    try:
        response = tv_device.handle_request(
            method="POST",
            path="/upnp/control",
            headers=headers,
            body=body
        )
        
        status_code = response["status"]
        
        # Auth must be checked
        assert "auth" in call_order, (
            "Authentication check was never called - endpoint has no auth!"
        )
        
        # If auth failed (401/403), string processing must NOT have occurred
        if status_code in (401, 403):
            assert "string_processing" not in call_order, (
                f"CRITICAL: UPnP string processing occurred BEFORE or DESPITE auth rejection! "
                f"Call order: {call_order}. "
                f"This means malicious UPnP strings could reach unsafe strcpy calls "
                f"even for unauthenticated requests."
            )
        
        # Auth must be first in call order if both were called
        if "auth" in call_order and "string_processing" in call_order:
            auth_idx = call_order.index("auth")
            string_idx = call_order.index("string_processing")
            assert auth_idx < string_idx, (
                f"Auth check ({auth_idx}) did not precede string processing ({string_idx}). "
                f"Call order: {call_order}"
            )
    finally:
        # Restore original methods
        tv_device._validate_auth = original_validate_auth
        tv_device._validate_upnp_string = original_validate_string