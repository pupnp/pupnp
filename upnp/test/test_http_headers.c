/* test_http_headers.c
 *
 * Regression test for issue #347: HTTP response headers must use
 * conventional title-case names (Content-Length, not CONTENT-LENGTH, etc.)
 *
 * Starts the UPnP HTTP server via UpnpInit2, sends a GET request over a raw
 * TCP socket, reads the response headers, and verifies title-case casing.
 *
 * These tests are expected to FAIL until the fix is implemented.
 * regression: issue #347
 */

#include "upnp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
	#include <arpa/inet.h>
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <unistd.h>

/* Read HTTP response headers into a malloc'd buffer.
 * Returns NULL on error.  Caller must free(). */
static char *get_response_headers(const char *server_ip, unsigned short port)
{
	int fd = -1;
	struct sockaddr_in addr;
	struct timeval tv;
	char req[256];
	char *buf;
	int n_total = 0;
	ssize_t n;
	const int BUF_LEN = 4096;

	buf = malloc((size_t)BUF_LEN);
	if (!buf)
		return NULL;

	snprintf(req,
		sizeof(req),
		"GET / HTTP/1.1\r\nHost: %s:%u\r\nConnection: close\r\n\r\n",
		server_ip,
		(unsigned)port);

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		free(buf);
		return NULL;
	}

	tv.tv_sec = 5;
	tv.tv_usec = 0;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if (inet_pton(AF_INET, server_ip, &addr.sin_addr) != 1)
		goto fail;
	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0)
		goto fail;
	if (send(fd, req, strlen(req), 0) < 0)
		goto fail;

	/* Read until the end of the header section. */
	while (n_total < BUF_LEN - 1) {
		n = recv(fd, buf + n_total, (size_t)(BUF_LEN - 1 - n_total), 0);
		if (n <= 0)
			break;
		n_total += (int)n;
		buf[n_total] = '\0';
		if (strstr(buf, "\r\n\r\n"))
			break;
	}
	buf[n_total] = '\0';
	close(fd);
	return buf;

fail:
	close(fd);
	free(buf);
	return NULL;
}

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
			"FAIL: missing title-case header \"%s\"\n",
			needle);
		return -1;
	}
	return 0;
}

#endif /* !_WIN32 */

int main(void)
{
#ifdef _WIN32
	puts("SKIP: raw POSIX socket test not supported on Windows.");
	return EXIT_SUCCESS;
#else
	int rc;
	const char *server_ip;
	unsigned short server_port;
	char *headers;
	int result = 0;

	rc = UpnpInit2(NULL, 0);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr,
			"UpnpInit2 failed (%d); skipping (no network?)\n",
			rc);
		return EXIT_SUCCESS;
	}

	server_ip = UpnpGetServerIpAddress();
	server_port = UpnpGetServerPort();
	if (!server_ip || !server_port) {
		fprintf(stderr, "Could not determine server address\n");
		UpnpFinish();
		return EXIT_FAILURE;
	}

	headers = get_response_headers(server_ip, server_port);
	if (!headers) {
		fprintf(stderr, "Failed to fetch HTTP response headers\n");
		UpnpFinish();
		return EXIT_FAILURE;
	}

	printf("Response headers received:\n%s\n", headers);

	/* Must NOT contain all-uppercase header names (current behaviour). */
	result |= check_absent(headers, "SERVER: ");
	result |= check_absent(headers, "CONTENT-LENGTH: ");
	result |= check_absent(headers, "CONTENT-TYPE: ");
	result |= check_absent(headers, "CONNECTION: ");

	/* Must contain conventional title-case header names (after fix). */
	result |= check_present(headers, "Server: ");
	result |= check_present(headers, "Content-Length: ");
	result |= check_present(headers, "Content-Type: ");
	result |= check_present(headers, "Connection: ");

	free(headers);
	UpnpFinish();

	if (result == 0)
		puts("All HTTP header capitalisation tests passed.");
	return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
#endif /* !_WIN32 */
}
