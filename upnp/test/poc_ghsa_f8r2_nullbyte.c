/* poc_ghsa_f8r2_nullbyte.c
 *
 * Regression test for GHSA-279g-pwr2-f8r2: embedded NUL byte (%00)
 * injection in HTTP request paths bypasses path handling.
 *
 * process_request() in webserver.c percent-decodes the request path with
 * remove_escaped_chars(), which correctly turns "%00" into a literal NUL
 * byte and tracks the true (post-decode) length in a separate size_t.
 * Everything downstream, however, treats the decoded buffer as a plain
 * C string (strcmp/strncmp/strlen in remove_dots(), isFileInVirtualDir(),
 * get_alias(), membuffer_assign_str()), so any bytes after the embedded
 * NUL are silently invisible to those checks.
 *
 * This lets a request for "/desc.xml%00.png" match an alias registered
 * as "/desc.xml", even though the two are different byte sequences on
 * the wire -- exactly the kind of internal/external request-path mismatch
 * a WAF or reverse-proxy extension filter would not expect.
 *
 * Pre-fix: server returns 200 (silently matches the truncated alias).
 * Post-fix: server rejects the request instead of matching the alias.
 *
 * regression: GHSA-279g-pwr2-f8r2
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

/* regression: GHSA-279g-pwr2-f8r2 -- test hook exported from libupnp */
extern int web_server_ut_set_alias(
	const char *name, const char *content, size_t len);

	#ifndef MSG_NOSIGNAL
		#define MSG_NOSIGNAL 0
	#endif

/* The miniserver rejects requests whose Host header isn't a numeric
 * ip:port (DNS-rebinding protection), so the Host header below must
 * match the server's own address. */
static int get_status_code(const char *ip, unsigned short port,
	const char *path)
{
	int sock;
	struct sockaddr_in addr;
	char req[256];
	char buf[512];
	ssize_t n;
	size_t total = 0;
	int status = -1;

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

	snprintf(req,
		sizeof req,
		"GET %s HTTP/1.1\r\n"
		"Host: %s:%u\r\n"
		"Connection: close\r\n\r\n",
		path,
		ip,
		port);
	send(sock, req, strlen(req), MSG_NOSIGNAL);

	while (total < sizeof buf - 1 &&
		(n = recv(sock, buf + total, sizeof buf - 1 - total, 0)) >
			0) {
		total += (size_t)n;
	}
	buf[total] = '\0';
	close(sock);

	if (total > 0) {
		sscanf(buf, "HTTP/%*d.%*d %d", &status);
	}

	return status;
}

int main(void)
{
	int rc;
	char *ip;
	unsigned short port;
	int status;

	rc = UpnpInit2(NULL, 0);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr,
			"UpnpInit2 failed (%d); skipping (no network?)\n",
			rc);
		return EXIT_SUCCESS;
	}

	rc = web_server_ut_set_alias("/desc.xml", "<root/>", 7);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr, "web_server_ut_set_alias failed (%d)\n", rc);
		UpnpFinish();
		return EXIT_FAILURE;
	}

	ip = UpnpGetServerIpAddress();
	port = UpnpGetServerPort();
	if (ip == NULL || port == 0) {
		fprintf(stderr,
			"UpnpGetServerIpAddress/UpnpGetServerPort returned "
			"nothing; skipping\n");
		UpnpFinish();
		return EXIT_SUCCESS;
	}

	/* Sanity check: the plain alias must resolve, otherwise the '%00'
	 * check below would be meaningless. */
	if (get_status_code(ip, port, "/desc.xml") != 200) {
		fprintf(stderr,
			"Baseline request for '/desc.xml' did not return "
			"200; skipping (environment issue?)\n");
		UpnpFinish();
		return EXIT_SUCCESS;
	}

	/* "%00" decodes to a NUL byte; the registered alias is "/desc.xml",
	 * not "/desc.xml\0.png", so this must NOT be served as the alias. */
	status = get_status_code(ip, port, "/desc.xml%00.png");

	UpnpFinish();

	if (status == 200) {
		fprintf(stderr,
			"FAIL: server returned 200 for '/desc.xml%%00.png' "
			"-- embedded NUL byte truncated the path to match "
			"the '/desc.xml' alias (GHSA-279g-pwr2-f8r2)\n");
		return EXIT_FAILURE;
	}

	printf("PASS: request with embedded NUL byte was not silently "
	       "truncated to match the alias (status=%d)\n",
		status);
	return EXIT_SUCCESS;
}

#else /* _WIN32 */

int main(void)
{
	puts("SKIP: test uses POSIX sockets (not available on Windows).");
	return EXIT_SUCCESS;
}

#endif /* _WIN32 */
