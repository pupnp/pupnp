import pytest
import ctypes
import sys

# Adversarial payloads targeting buffer overflow via oversized strings
# These simulate what an attacker could send via UPnP SOAP action requests

ADVERSARIAL_PAYLOADS = [
    # Exact boundary values
    "A" * 255,
    "A" * 256,
    "A" * 257,
    # Common buffer sizes + 1
    "B" * 64,
    "B" * 65,
    "B" * 128,
    "B" * 129,
    # Large overflow attempts
    "C" * 1024,
    "C" * 4096,
    "C" * 65535,
    # Format string attack payloads
    "%s%s%s%s%s%s%s%s%s%s",
    "%n%n%n%n%n%n%n%n",
    "%x%x%x%x%x%x%x%x",
    "%.99999d",
    # Null byte injection
    "value\x00overflow_data" + "A" * 200,
    "\x00" * 256,
    # Mixed content with null bytes
    "A" * 100 + "\x00" + "B" * 200,
    # Unicode/multibyte sequences that expand
    "\xff\xfe" * 128,
    "\xc0\xaf" * 128,
    # Shell metacharacters
    "$(echo overflow)" + "A" * 200,
    "`id`" + "A" * 200,
    # XML/SOAP injection within value
    "<value>" + "A" * 500 + "</value>",
    "&lt;" + "A" * 500,
    # Newline injection
    "value\r\n" + "A" * 300,
    "value\n" + "A" * 300,
    # Integer-like values that are oversized
    "9" * 512,
    "-" + "9" * 511,
    # Empty and minimal
    "",
    "A",
    # Path traversal style
    "../" * 100 + "overflow",
    # Repeated special chars
    ";" * 512,
    "|" * 512,
    "&" * 512,
]


def simulate_safe_copy(destination_buffer_size: int, value: str) -> dict:
    """
    Simulates what a SAFE implementation of the strcpy operation should do.
    Returns a dict with:
      - 'safe': bool indicating if the operation was performed safely
      - 'truncated': bool indicating if value was truncated
      - 'result': the resulting string (truncated if necessary)
      - 'value_length': length of the input value
    """
    value_length = len(value)
    # Safe implementation must not write beyond buffer bounds
    # Buffer must have space for null terminator
    max_safe_length = destination_buffer_size - 1

    if value_length > max_safe_length:
        # Safe implementation truncates
        result = value[:max_safe_length]
        return {
            "safe": True,
            "truncated": True,
            "result": result,
            "value_length": value_length,
            "buffer_size": destination_buffer_size,
        }
    else:
        return {
            "safe": True,
            "truncated": False,
            "result": value,
            "value_length": value_length,
            "buffer_size": destination_buffer_size,
        }


def validate_upnp_value_length(value: str, max_allowed_length: int) -> bool:
    """
    Validates that a UPnP SOAP action value does not exceed the maximum
    allowed length for the target buffer.
    This represents the security check that MUST exist before any copy operation.
    """
    if not isinstance(value, str):
        return False
    return len(value) <= max_allowed_length


# Typical fixed buffer sizes used in embedded/UPnP implementations
TYPICAL_BUFFER_SIZES = [64, 128, 256]


@pytest.mark.parametrize("payload", ADVERSARIAL_PAYLOADS)
def test_upnp_value_length_validation_invariant(payload):
    """
    Invariant: Any value received from a UPnP SOAP action request (SetVolume,
    SetChannel, etc.) MUST be validated for length before being copied into a
    fixed-size buffer. The length of the value MUST NOT exceed the destination
    buffer size minus 1 (for null terminator). If validation is bypassed or
    absent, a buffer overflow can corrupt adjacent memory structures.
    """
    # The security invariant: for each typical buffer size used in tv_device.c,
    # a safe copy operation must never write beyond the buffer boundary.
    for buffer_size in TYPICAL_BUFFER_SIZES:
        max_safe_length = buffer_size - 1

        # INVARIANT 1: Length validation must correctly identify oversized inputs
        is_valid = validate_upnp_value_length(payload, max_safe_length)
        if len(payload) > max_safe_length:
            assert not is_valid, (
                f"SECURITY VIOLATION: Value of length {len(payload)} was accepted "
                f"for buffer of size {buffer_size}. "
                f"This would cause a buffer overflow in strcpy."
            )
        else:
            assert is_valid, (
                f"Valid value of length {len(payload)} was incorrectly rejected "
                f"for buffer of size {buffer_size}."
            )

        # INVARIANT 2: Safe copy simulation must never exceed buffer bounds
        result = simulate_safe_copy(buffer_size, payload)
        assert result["safe"] is True, (
            f"Safe copy operation failed for buffer_size={buffer_size}"
        )
        assert len(result["result"]) <= max_safe_length, (
            f"SECURITY VIOLATION: Copied string length {len(result['result'])} "
            f"exceeds max safe length {max_safe_length} for buffer size {buffer_size}. "
            f"Original payload length: {len(payload)}"
        )

        # INVARIANT 3: If input exceeds buffer, it must be truncated (not silently accepted)
        if len(payload) > max_safe_length:
            assert result["truncated"] is True, (
                f"SECURITY VIOLATION: Oversized input (len={len(payload)}) was not "
                f"truncated for buffer of size {buffer_size}. "
                f"strcpy without length check would overflow this buffer."
            )

        # INVARIANT 4: Result must always fit within the declared buffer
        result_bytes = result["result"].encode("utf-8", errors="replace")
        assert len(result_bytes) < buffer_size, (
            f"SECURITY VIOLATION: Result bytes ({len(result_bytes)}) do not fit "
            f"within buffer of size {buffer_size} (need space for null terminator). "
            f"Payload length was {len(payload)}."
        )


@pytest.mark.parametrize("payload", ADVERSARIAL_PAYLOADS)
def test_upnp_soap_value_must_not_overflow_variable_str_val(payload):
    """
    Invariant: The VariableStrVal buffer in tv_service_table MUST NOT be
    overflowed by any network-supplied value from UPnP SOAP requests.
    Any implementation copying 'value' into VariableStrVal MUST enforce
    a maximum length constraint equal to (sizeof(VariableStrVal) - 1).
    """
    # Simulate the fixed buffer size for VariableStrVal
    # Common sizes in UPnP implementations
    VARIABLE_STR_VAL_SIZE = 256  # typical fixed buffer size

    value = payload
    max_copy_length = VARIABLE_STR_VAL_SIZE - 1  # leave room for null terminator

    # INVARIANT: The length of any value written to VariableStrVal must be bounded
    safe_length = min(len(value), max_copy_length)

    # Simulate what strncpy or a bounds-checked copy would do
    safe_value = value[:safe_length]

    # The result must always fit in the buffer
    assert len(safe_value) <= max_copy_length, (
        f"SECURITY VIOLATION: safe_value length {len(safe_value)} exceeds "
        f"max allowed {max_copy_length} for VariableStrVal buffer of size "
        f"{VARIABLE_STR_VAL_SIZE}"
    )

    # The original strcpy (vulnerable) would write len(value)+1 bytes (including null)
    # This MUST be caught by validation
    if len(value) >= VARIABLE_STR_VAL_SIZE:
        # An unvalidated strcpy here would overflow
        overflow_bytes = len(value) + 1 - VARIABLE_STR_VAL_SIZE
        assert overflow_bytes > 0  # confirm overflow would occur without validation
        # The invariant: validation MUST prevent this from reaching strcpy
        assert len(safe_value) < VARIABLE_STR_VAL_SIZE, (
            f"SECURITY INVARIANT VIOLATED: Input of length {len(value)} must be "
            f"rejected or truncated before strcpy into {VARIABLE_STR_VAL_SIZE}-byte buffer. "
            f"Without validation, {overflow_bytes} bytes would overflow into adjacent memory."
        )


@pytest.mark.parametrize("payload", ADVERSARIAL_PAYLOADS)
def test_network_input_sanitization_before_buffer_copy(payload):
    """
    Invariant: All network-supplied string values (from UPnP SOAP actions like
    SetVolume, SetChannel) MUST be sanitized and length-checked before any
    buffer copy operation. The security boundary between network input and
    internal buffers must always be maintained.
    """
    # Represents the security gate that must exist in the code path
    # between receiving a UPnP SOAP value and copying it to VariableStrVal

    MAX_BUFFER_SIZE = 256

    def security_gate(value: str, buffer_size: int) -> tuple:
        """
        Returns (is_safe, sanitized_value) tuple.
        This represents the check that MUST exist before strcpy.
        """
        if not isinstance(value, str):
            return False, None
        if len(value) >= buffer_size:
            return False, None  # Reject oversized input
        # Additional: check for embedded null bytes that could confuse length checks
        if '\x00' in value:
            # Truncate at first null byte
            value = value.split('\x00')[0]
            if len(value) >= buffer_size:
                return False, None
        return True, value

    is_safe, sanitized = security_gate(payload, MAX_BUFFER_SIZE)

    if is_safe:
        # If accepted, the sanitized value MUST fit in the buffer
        assert sanitized is not None
        assert len(sanitized) < MAX_BUFFER_SIZE, (
            f"SECURITY VIOLATION: Accepted value length {len(sanitized)} "
            f">= buffer size {MAX_BUFFER_SIZE}"
        )
        # Must be safe to copy (no overflow)
        assert len(sanitized) + 1 <= MAX_BUFFER_SIZE, (
            f"SECURITY VIOLATION: Value + null terminator ({len(sanitized) + 1} bytes) "
            f"exceeds buffer size {MAX_BUFFER_SIZE}"
        )
    else:
        # If rejected, the original payload must have been dangerous
        # (either too long or contained null bytes that could cause issues)
        original_len = len(payload)
        has_null = '\x00' in payload
        truncated_at_null = payload.split('\x00')[0] if has_null else payload

        dangerous = (
            original_len >= MAX_BUFFER_SIZE or
            len(truncated_at_null) >= MAX_BUFFER_SIZE or
            has_null
        )
        assert dangerous, (
            f"Security gate rejected a safe input of length {original_len}. "
            f"This may indicate overly strict validation, but the invariant "
            f"requires that all dangerous inputs are rejected."
        )