/* test_webserver_post.c
 *
 * The web server accepts a POST only for a virtual directory: the body goes
 * to the open/write/close callbacks, and the reply is 200. A POST to a file
 * in the document root is answered with 404 and leaves the file untouched
 * (filesystem writes on POST were removed after CVE-2016-6255).
 *
 * The body is longer than the 1024-byte chunks that http_RecvPostMessage()
 * hands to the write callback, so more than one write is needed.
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

	#define BODY_LEN 3000

static char g_received[BODY_LEN + 1];
static size_t g_received_len;
static int g_opened_for_write;
static int g_writes;
static int g_closes;
static int g_handle;

static int get_info(const char *filename,
	UpnpFileInfo *info,
	const void *cookie,
	const void **request_cookie)
{
	(void)filename;
	(void)cookie;
	(void)request_cookie;
	UpnpFileInfo_set_FileLength(info, 0);
	UpnpFileInfo_set_IsReadable(info, 1);
	UpnpFileInfo_set_IsDirectory(info, 0);
	return 0;
}

static UpnpWebFileHandle vd_open(const char *filename,
	enum UpnpOpenFileMode mode,
	const void *cookie,
	const void *request_cookie)
{
	(void)filename;
	(void)cookie;
	(void)request_cookie;
	if (mode == UPNP_WRITE)
		g_opened_for_write++;
	return &g_handle;
}

static int vd_write(UpnpWebFileHandle h,
	char *buf,
	size_t len,
	const void *cookie,
	const void *request_cookie)
{
	(void)cookie;
	(void)request_cookie;
	if (h != &g_handle || g_received_len + len > BODY_LEN)
		return -1;
	memcpy(g_received + g_received_len, buf, len);
	g_received_len += len;
	g_writes++;
	return (int)len;
}

static int vd_close(
	UpnpWebFileHandle h, const void *cookie, const void *request_cookie)
{
	(void)cookie;
	(void)request_cookie;
	if (h == &g_handle)
		g_closes++;
	return 0;
}

/* Sends a POST of body to path. Returns the status code, or -1. */
static int post(const char *ip,
	unsigned short port,
	const char *path,
	const char *body,
	size_t body_len)
{
	int sock;
	struct sockaddr_in addr;
	char req[256];
	char buf[1024];
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
		"POST %s HTTP/1.1\r\n"
		"Host: %s:%u\r\n"
		"Content-Type: application/octet-stream\r\n"
		"Content-Length: %u\r\n"
		"Connection: close\r\n\r\n",
		path,
		ip,
		port,
		(unsigned)body_len);
	send(sock, req, strlen(req), MSG_NOSIGNAL);
	send(sock, body, body_len, MSG_NOSIGNAL);
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

int main(void)
{
	int rc;
	int err = 0;
	int status;
	const char *ip;
	unsigned short port;
	char root[] = "/tmp/upnp_post_XXXXXX";
	char file[sizeof root + 16];
	char body[BODY_LEN];
	char check[16];
	FILE *f;
	size_t i;

	for (i = 0; i < BODY_LEN; i++)
		body[i] = (char)('a' + i % 26);

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
	snprintf(file, sizeof file, "%s/file.txt", root);
	f = fopen(file, "w");
	if (!f) {
		perror("fopen");
		rmdir(root);
		UpnpFinish();
		return EXIT_FAILURE;
	}
	fputs("original", f);
	fclose(f);
	if (UpnpSetWebServerRootDir(root) != UPNP_E_SUCCESS ||
		UpnpVirtualDir_set_GetInfoCallback(get_info) !=
			UPNP_E_SUCCESS ||
		UpnpVirtualDir_set_OpenCallback(vd_open) != UPNP_E_SUCCESS ||
		UpnpVirtualDir_set_WriteCallback(vd_write) != UPNP_E_SUCCESS ||
		UpnpVirtualDir_set_CloseCallback(vd_close) != UPNP_E_SUCCESS ||
		UpnpAddVirtualDir("/vd", NULL, NULL) != UPNP_E_SUCCESS) {
		fprintf(stderr, "web server setup failed\n");
		err = 1;
	}

	if (!err) {
		/* POST to a virtual directory: the body reaches the callbacks.
		 */
		status = post(ip, port, "/vd/up.bin", body, BODY_LEN);
		printf("POST /vd/up.bin -> %d, %zu bytes in %d writes, "
		       "%d open(s) for write, %d close(s)\n",
			status,
			g_received_len,
			g_writes,
			g_opened_for_write,
			g_closes);
		if (status != 200 || g_opened_for_write != 1 ||
			g_received_len != BODY_LEN ||
			memcmp(g_received, body, BODY_LEN) != 0 ||
			g_writes < 2 || g_closes != 1) {
			fprintf(stderr, "  FAIL: virtual POST\n");
			err = 1;
		}

		/* POST to a file in the document root: 404, file untouched. */
		status = post(ip, port, "/file.txt", body, 64);
		f = fopen(file, "r");
		memset(check, 0, sizeof check);
		if (f) {
			if (fgets(check, sizeof check, f) == NULL)
				check[0] = '\0';
			fclose(f);
		}
		printf("POST /file.txt -> %d, file now '%s'\n", status, check);
		if (status != 404 || strcmp(check, "original") != 0) {
			fprintf(stderr, "  FAIL: POST to a real file\n");
			err = 1;
		}
	}

	UpnpFinish();
	unlink(file);
	rmdir(root);
	if (!err)
		puts("test_webserver_post: PASS");

	return err ? EXIT_FAILURE : EXIT_SUCCESS;
}

#else /* _WIN32 */

int main(void)
{
	puts("SKIP: test uses POSIX sockets (not available on Windows).");
	return EXIT_SUCCESS;
}

#endif /* _WIN32 */
