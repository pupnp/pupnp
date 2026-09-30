/* test_webserver_alias_leak.c
 *
 * Regression test: process_request() must release the alias reference it
 * takes.
 *
 * process_request() grabs a reference on the installed alias document for
 * every request that is not served from a virtual directory, but it only
 * gave it back on errors; the one success path that hands the alias on
 * (a GET answered from the alias) releases it in web_server_callback().
 * A GET or HEAD of a regular file, or a HEAD of the alias itself, leaked
 * a reference. The document then survived web_server_set_alias(NULL) and
 * kept being served after it was uninstalled.
 *
 * Each case installs the alias, makes one request, uninstalls the alias and
 * expects a request for it to fail with 404.
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

static int get_status_code(const char *ip,
	unsigned short port,
	const char *method,
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
	/* The miniserver only accepts a numeric ip:port Host header. */
	snprintf(req,
		sizeof req,
		"%s %s HTTP/1.1\r\n"
		"Host: %s:%u\r\n"
		"Connection: close\r\n\r\n",
		method,
		path,
		ip,
		port);
	send(sock, req, strlen(req), MSG_NOSIGNAL);
	while (total < sizeof buf - 1 &&
		(n = recv(sock, buf + total, sizeof buf - 1 - total, 0)) > 0) {
		total += (size_t)n;
	}
	buf[total] = '\0';
	close(sock);
	if (total > 0)
		sscanf(buf, "HTTP/%*d.%*d %d", &status);

	return status;
}

/* Install the alias, make one request, uninstall the alias, then request the
 * alias. Returns 0 when the alias is gone, 1 when it is still served. */
static int check_case(const char *ip,
	unsigned short port,
	const char *method,
	const char *path)
{
	int status;

	if (web_server_ut_set_alias("/desc.xml", "<root/>", 7) !=
		UPNP_E_SUCCESS) {
		fprintf(stderr, "web_server_ut_set_alias failed\n");
		return 1;
	}
	status = get_status_code(ip, port, method, path);
	printf("%s %s -> %d\n", method, path, status);
	if (status != 200) {
		fprintf(stderr, "  baseline request failed: environment?\n");
		web_server_ut_set_alias(NULL, NULL, 0);
		return 1;
	}
	web_server_ut_set_alias(NULL, NULL, 0);
	status = get_status_code(ip, port, "GET", "/desc.xml");
	printf("  after uninstall: GET /desc.xml -> %d\n", status);
	if (status == 200) {
		fprintf(stderr,
			"FAIL: alias still served after it was uninstalled\n");
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
	char root[] = "/tmp/upnp_alias_leak_XXXXXX";
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
	fputs("hello\n", f);
	fclose(f);
	if (UpnpSetWebServerRootDir(root) != UPNP_E_SUCCESS) {
		fprintf(stderr, "UpnpSetWebServerRootDir failed\n");
		err = 1;
	}

	if (!err) {
		err |= check_case(ip, port, "GET", "/hello.txt");
		err |= check_case(ip, port, "HEAD", "/hello.txt");
		err |= check_case(ip, port, "HEAD", "/desc.xml");
	}

	UpnpFinish();
	unlink(file);
	rmdir(root);
	if (!err)
		puts("test_webserver_alias_leak: PASS");

	return err ? EXIT_FAILURE : EXIT_SUCCESS;
}

#else /* _WIN32 */

int main(void)
{
	puts("SKIP: test uses POSIX sockets (not available on Windows).");
	return EXIT_SUCCESS;
}

#endif /* _WIN32 */
