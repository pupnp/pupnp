/* test_webserver_range.c
 *
 * Regression test: a Range request that starts at or after the end of the
 * file must be answered with 416.
 *
 * CreateHTTPRangeResponseHeader() compared the first byte with the file
 * length using '<', so for a 6-byte file "Range: bytes=6-10" (first byte 6,
 * one past the last byte) was accepted. The last byte was then clamped to
 * 5 and the reply was "206 Partial Content" with the impossible header
 * "Content-Range: bytes 6-5/6".
 *
 * The 416 reply must also tell the length of the file in a Content-Range
 * header with an asterisk in place of the range (RFC 7233, section 4.4).
 *
 * The valid ranges next to it are checked too, so the fix cannot reject
 * more than it should.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32

	#include "upnp.h"

	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <unistd.h>

	#ifndef MSG_NOSIGNAL
		#define MSG_NOSIGNAL 0
	#endif

/* Sends a GET with the given Range header. Returns the status code, or -1,
 * and copies the Content-Range header line (if any) to content_range. */
static int get_range(const char *ip,
	unsigned short port,
	const char *range,
	char *content_range,
	size_t content_range_size)
{
	int sock;
	struct sockaddr_in addr;
	char req[256];
	char buf[1024];
	char *p;
	ssize_t n;
	size_t total = 0;
	int status = -1;

	content_range[0] = '\0';
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		return -1;
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, ip, &addr.sin_addr);
	if (connect(sock, (struct sockaddr *)&addr, sizeof addr) != 0) {
		close(sock);
		return -1;
	}
	/* The miniserver only accepts a numeric ip:port Host header. */
	snprintf(req,
		sizeof req,
		"GET /hello.txt HTTP/1.1\r\n"
		"Host: %s:%u\r\n"
		"Range: %s\r\n"
		"Connection: close\r\n\r\n",
		ip,
		port,
		range);
	send(sock, req, strlen(req), MSG_NOSIGNAL);
	while (total < sizeof buf - 1 &&
		(n = recv(sock, buf + total, sizeof buf - 1 - total, 0)) > 0) {
		total += (size_t)n;
	}
	buf[total] = '\0';
	close(sock);
	if (total > 0)
		sscanf(buf, "HTTP/%*d.%*d %d", &status);
	p = strstr(buf, "Content-Range: ");
	if (p) {
		size_t len = strcspn(p, "\r\n");

		if (len >= content_range_size)
			len = content_range_size - 1;
		memcpy(content_range, p, len);
		content_range[len] = '\0';
	}

	return status;
}

/* expect_status 416 or 206, and the exact Content-Range line. */
static int check_range(const char *ip,
	unsigned short port,
	const char *range,
	int expect_status,
	const char *expect_content_range)
{
	char cr[128];
	int status = get_range(ip, port, range, cr, sizeof cr);

	printf("Range: %-14s -> %d %s\n", range, status, cr);
	if (status != expect_status) {
		fprintf(stderr, "  FAIL: expected status %d\n", expect_status);
		return 1;
	}
	if (expect_content_range && strcmp(cr, expect_content_range) != 0) {
		fprintf(stderr,
			"  FAIL: expected '%s'\n",
			expect_content_range);
		return 1;
	}

	return 0;
}

int main(void)
{
	int rc;
	int err = 0;
	const char *ip;
	unsigned short port;
	char root[] = "/tmp/upnp_range_XXXXXX";
	char file[sizeof root + 16];
	FILE *f;

	rc = UpnpInit2(NULL, 0);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr,
			"UpnpInit2 failed (%d); skipping (no network?)\n",
			rc);
		return EXIT_SUCCESS;
	}
	ip = UpnpGetServerIpAddress();
	port = UpnpGetServerPort();
	if (!ip || !port) {
		fprintf(stderr, "no server address; skipping\n");
		UpnpFinish();
		return EXIT_SUCCESS;
	}
	if (!mkdtemp(root)) {
		perror("mkdtemp");
		UpnpFinish();
		return EXIT_FAILURE;
	}
	snprintf(file, sizeof file, "%s/hello.txt", root);
	f = fopen(file, "w");
	if (!f) {
		perror("fopen");
		rmdir(root);
		UpnpFinish();
		return EXIT_FAILURE;
	}
	fputs("hello\n", f); /* 6 bytes */
	fclose(f);
	if (UpnpSetWebServerRootDir(root) != UPNP_E_SUCCESS) {
		fprintf(stderr, "UpnpSetWebServerRootDir failed\n");
		err = 1;
	}

	if (!err) {
		/* Valid ranges, including the last byte of the file. */
		err |= check_range(ip,
			port,
			"bytes=0-2",
			206,
			"Content-Range: bytes 0-2/6");
		err |= check_range(ip,
			port,
			"bytes=5-10",
			206,
			"Content-Range: bytes 5-5/6");
		err |= check_range(ip,
			port,
			"bytes=5-",
			206,
			"Content-Range: bytes 5-5/6");
		err |= check_range(ip,
			port,
			"bytes=-3",
			206,
			"Content-Range: bytes 3-5/6");
		/* Ranges that start at or after the end: not satisfiable. */
		err |= check_range(ip,
			port,
			"bytes=6-10",
			416,
			"Content-Range: bytes */6");
		err |= check_range(
			ip, port, "bytes=6-6", 416, "Content-Range: bytes */6");
		err |= check_range(ip,
			port,
			"bytes=7-10",
			416,
			"Content-Range: bytes */6");
		err |= check_range(
			ip, port, "bytes=6-", 416, "Content-Range: bytes */6");
	}

	UpnpFinish();
	unlink(file);
	rmdir(root);
	if (!err)
		puts("test_webserver_range: PASS");

	return err ? EXIT_FAILURE : EXIT_SUCCESS;
}

#else /* _WIN32 */

int main(void)
{
	puts("SKIP: test uses POSIX sockets (not available on Windows).");
	return EXIT_SUCCESS;
}

#endif /* _WIN32 */
