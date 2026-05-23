/* test_http_headers.c
 *
 * Regression test for issue #347: HTTP response headers must use
 * conventional title-case names (Content-Length, not CONTENT-LENGTH, etc.)
 *
 * These tests are expected to FAIL until the fix is implemented.
 * regression: issue #347
 */

#include "httpreadwrite.h"
#include "membuffer.h"

#include <stdio.h>
#include <string.h>

static int check_absent(const char *buf, const char *needle)
{
	if (strstr(buf, needle)) {
		fprintf(stderr,
			"FAIL: found uppercase header \"%s\"\n",
			needle);
		return -1;
	}
	return 0;
}

static int check_present(const char *buf, const char *needle)
{
	if (!strstr(buf, needle)) {
		fprintf(stderr,
			"FAIL: missing expected title-case header \"%s\"\n",
			needle);
		return -1;
	}
	return 0;
}

/* Test A: Date, Server, Content-Length, Content-Type headers via
 * http_MakeMessage format "RDSNC" (response + date + server +
 * content-length + content-type). */
static int test_response_headers_title_case(void)
{
	membuffer buf;
	off_t content_len = 100;
	int rc = 0;

	membuffer_init(&buf);

	if (http_MakeMessage(
		    &buf, 1, 1, "RDSNTc", 200, content_len, "text/html") != 0) {
		fprintf(stderr, "test A: http_MakeMessage failed\n");
		membuffer_destroy(&buf);
		return -1;
	}

	/* Must NOT contain all-uppercase variants */
	rc |= check_absent(buf.buf, "DATE: ");
	rc |= check_absent(buf.buf, "SERVER: ");
	rc |= check_absent(buf.buf, "CONTENT-LENGTH: ");
	rc |= check_absent(buf.buf, "CONTENT-TYPE: ");

	/* Must contain title-case variants */
	rc |= check_present(buf.buf, "Date: ");
	rc |= check_present(buf.buf, "Server: ");
	rc |= check_present(buf.buf, "Content-Length: ");
	rc |= check_present(buf.buf, "Content-Type: ");

	membuffer_destroy(&buf);
	return rc;
}

/* Test B: Connection: close header (format 'C', HTTP/1.1 only). */
static int test_connection_close_title_case(void)
{
	membuffer buf;
	int rc = 0;

	membuffer_init(&buf);

	if (http_MakeMessage(&buf, 1, 1, "C") != 0) {
		fprintf(stderr, "test B: http_MakeMessage 'C' failed\n");
		membuffer_destroy(&buf);
		return -1;
	}

	rc |= check_absent(buf.buf, "CONNECTION: ");
	rc |= check_present(buf.buf, "Connection: ");

	membuffer_destroy(&buf);
	return rc;
}

/* Test C: Transfer-Encoding: chunked header (format 'K'). */
static int test_transfer_encoding_title_case(void)
{
	membuffer buf;
	int rc = 0;

	membuffer_init(&buf);

	if (http_MakeMessage(&buf, 1, 1, "K") != 0) {
		fprintf(stderr, "test C: http_MakeMessage 'K' failed\n");
		membuffer_destroy(&buf);
		return -1;
	}

	rc |= check_absent(buf.buf, "TRANSFER-ENCODING: ");
	rc |= check_present(buf.buf, "Transfer-Encoding: ");

	membuffer_destroy(&buf);
	return rc;
}

int main(void)
{
	int rc = 0;

	rc |= test_response_headers_title_case();
	rc |= test_connection_close_title_case();
	rc |= test_transfer_encoding_title_case();

	if (rc == 0)
		puts("All HTTP header capitalisation tests passed.");

	return rc == 0 ? 0 : 1;
}
