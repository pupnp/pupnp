/* test_webserver_alias_range.c
 *
 * Regression test: the body of a response for the XML alias must match the
 * framing its headers announce.
 *
 * process_request() makes the headers for the alias from the Range and TE
 * headers of the request like for a file: "206 Partial Content" with the
 * length of the range, or "Transfer-Encoding: chunked". But
 * web_server_callback() sent the whole document as it was, so a GET with
 * "Range: bytes=0-9" got "Content-Length: 10" followed by the whole
 * document, and a GET with a TE header got a chunked response whose body
 * had no chunks.
 *
 * Each case checks the status, that the body has exactly the length in the
 * Content-Length header and that it holds the expected bytes.
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

/* Test hook exported from libupnp. */
extern int web_server_ut_set_alias(
	const char *name, const char *content, size_t len);

	#ifndef MSG_NOSIGNAL
		#define MSG_NOSIGNAL 0
	#endif

static const char doc[] = "<?xml version=\"1.0\"?>\n<root>0123456789</root>\n";

/* Sends a GET of the alias with one extra header line and checks the reply.
 * Returns 0 on success, 1 on failure. */
static int check_case(const char *ip,
	unsigned short port,
	const char *extra_header,
	int expect_status,
	const char *expect_body,
	size_t expect_len)
{
	int sock;
	struct sockaddr_in addr;
	char req[256];
	char buf[2048];
	char *body;
	char *p;
	ssize_t n;
	size_t total = 0;
	size_t body_len;
	int status = -1;
	long content_length = -1;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		return 1;
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, ip, &addr.sin_addr);
	if (connect(sock, (struct sockaddr *)&addr, sizeof addr) != 0) {
		close(sock);
		return 1;
	}
	/* The miniserver only accepts a numeric ip:port Host header. */
	snprintf(req,
		sizeof req,
		"GET /desc.xml HTTP/1.1\r\n"
		"Host: %s:%u\r\n"
		"%s\r\n"
		"Connection: close\r\n\r\n",
		ip,
		port,
		extra_header);
	send(sock, req, strlen(req), MSG_NOSIGNAL);
	while (total < sizeof buf - 1 &&
		(n = recv(sock, buf + total, sizeof buf - 1 - total, 0)) > 0) {
		total += (size_t)n;
	}
	buf[total] = '\0';
	close(sock);
	if (total > 0)
		sscanf(buf, "HTTP/%*d.%*d %d", &status);
	body = strstr(buf, "\r\n\r\n");
	if (!body) {
		fprintf(stderr, "%s: no end of headers\n", extra_header);
		return 1;
	}
	*body = '\0';
	body += 4;
	body_len = total - (size_t)(body - buf);
	p = strstr(buf, "Content-Length: ");
	if (p)
		content_length =
			strtol(p + strlen("Content-Length: "), NULL, 10);

	printf("%-22s -> %d, Content-Length %ld, %zu body bytes\n",
		extra_header,
		status,
		content_length,
		body_len);
	if (status != expect_status) {
		fprintf(stderr, "  FAIL: expected status %d\n", expect_status);
		return 1;
	}
	if (content_length < 0 || (size_t)content_length != body_len) {
		fprintf(stderr,
			"  FAIL: the body does not have the length "
			"announced\n");
		return 1;
	}
	if (body_len != expect_len || memcmp(body, expect_body, expect_len)) {
		fprintf(stderr, "  FAIL: unexpected body\n");
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
	size_t len = strlen(doc);

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
	if (web_server_ut_set_alias("/desc.xml", doc, len) != UPNP_E_SUCCESS) {
		fprintf(stderr, "web_server_ut_set_alias failed\n");
		UpnpFinish();
		return EXIT_FAILURE;
	}

	err |= check_case(ip, port, "X-Test: none", 200, doc, len);
	err |= check_case(ip, port, "Range: bytes=0-9", 206, doc, 10);
	err |= check_case(ip, port, "Range: bytes=-4", 206, doc + len - 4, 4);
	err |= check_case(
		ip, port, "Range: bytes=10-", 206, doc + 10, len - 10);
	err |= check_case(ip, port, "TE: trailers", 200, doc, len);

	web_server_ut_set_alias(NULL, NULL, 0);
	UpnpFinish();
	if (!err)
		puts("test_webserver_alias_range: PASS");

	return err ? EXIT_FAILURE : EXIT_SUCCESS;
}

#else /* _WIN32 */

int main(void)
{
	puts("SKIP: test uses POSIX sockets (not available on Windows).");
	return EXIT_SUCCESS;
}

#endif /* _WIN32 */
