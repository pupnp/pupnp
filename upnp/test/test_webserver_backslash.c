/* test_webserver_backslash.c
 *
 * Regression test: a backslash in a request path must not let a request
 * leave the web server's document root on Windows.
 *
 * process_request() decodes the request path and then removes the "." and
 * ".." segments with remove_dots(), which only knows '/' as a separator. A
 * backslash sent as %5c is decoded into a plain character, so "/..%5cx" is
 * handled as the single segment "..\x". The Windows file system treats the
 * backslash as a separator, and "<docroot>/..\x" is a file outside the
 * document root. On other systems a backslash is an ordinary file name
 * character and the same request only names a file that does not exist.
 *
 * The test serves a document root that has a file in it, puts a second file
 * next to the document root, and requests the second file with the ways
 * of writing "go up one directory" that use a backslash. None of them may
 * return the file. A plain request for the file inside the root and the
 * forward slash form are checked too, so that an answer of 404 for every
 * request cannot pass.
 *
 * On Windows a colon in a path names a drive or an NTFS alternate data
 * stream, so the test also requests a stream of the file in the root and
 * its default data stream, and neither may be served.
 */

#include "upnp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
	#include <direct.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
typedef SOCKET sock_t;
	#define SOCK_INVALID INVALID_SOCKET
	#define sock_close closesocket
	#define make_dir(p) _mkdir(p)
	#define remove_dir(p) _rmdir(p)
	#define remove_file(p) _unlink(p)
	#define get_cwd(b, n) _getcwd(b, (int)(n))
#else
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <sys/stat.h>
	#include <sys/time.h>
	#include <unistd.h>
typedef int sock_t;
	#define SOCK_INVALID (-1)
	#define sock_close close
	#define make_dir(p) mkdir(p, 0755)
	#define remove_dir(p) rmdir(p)
	#define remove_file(p) unlink(p)
	#define get_cwd(b, n) getcwd(b, n)
#endif

#ifndef MSG_NOSIGNAL
	#define MSG_NOSIGNAL 0
#endif

#define SECRET_TEXT "S3-SECRET-OUTSIDE-THE-DOCUMENT-ROOT"
#define INSIDE_TEXT "inside the document root"
#define TEST_DIR "s3_backslash_test"

static void set_recv_timeout(sock_t fd, int seconds)
{
#ifdef _WIN32
	DWORD ms = (DWORD)seconds * 1000;

	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&ms, sizeof ms);
#else
	struct timeval tv;

	tv.tv_sec = seconds;
	tv.tv_usec = 0;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
#endif
}

/* Requests path and returns the status code, or -1. *leaked is set when the
 * answer contains the text of the file outside the document root. */
static int get_path(
	const char *ip, unsigned short port, const char *path, int *leaked)
{
	sock_t fd;
	struct sockaddr_in addr;
	char req[512];
	char buf[4096];
	size_t total = 0;
	int n;
	int status = -1;

	*leaked = 0;
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == SOCK_INVALID)
		return -1;
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, ip, &addr.sin_addr);
	if (connect(fd, (struct sockaddr *)&addr, sizeof addr) != 0) {
		sock_close(fd);
		return -1;
	}
	set_recv_timeout(fd, 5);
	/* The miniserver only accepts a numeric ip:port Host header. */
	snprintf(req,
		sizeof req,
		"GET %s HTTP/1.1\r\n"
		"Host: %s:%u\r\n"
		"Connection: close\r\n\r\n",
		path,
		ip,
		port);
	send(fd, req, (int)strlen(req), MSG_NOSIGNAL);
	while (total < sizeof buf - 1 &&
		(n = recv(fd, buf + total, (int)(sizeof buf - 1 - total), 0)) >
			0) {
		total += (size_t)n;
	}
	buf[total] = '\0';
	sock_close(fd);
	if (strncmp(buf, "HTTP/", 5) == 0) {
		const char *space = strchr(buf, ' ');

		if (space)
			status = atoi(space + 1);
	}
	if (strstr(buf, SECRET_TEXT))
		*leaked = 1;

	return status;
}

static int write_file(const char *path, const char *text)
{
	FILE *f = fopen(path, "wb");

	if (!f)
		return -1;
	fputs(text, f);
	fclose(f);

	return 0;
}

/* Expects the request not to return the file outside the document root. */
static int check_blocked(const char *ip, unsigned short port, const char *path)
{
	int leaked;
	int status = get_path(ip, port, path, &leaked);

	printf("GET %-32s -> %d%s\n", path, status, leaked ? " LEAKED" : "");
	if (status == 200 || leaked) {
		fprintf(stderr, "FAIL: %s was served\n", path);
		return 1;
	}

	return 0;
}

int main(void)
{
	int rc;
	int err = 0;
	int leaked;
	const char *ip;
	unsigned short port;
	char cwd[1024];
	char root[1200];
	char docroot[1300];
	char sub[1400];
	char inside[1400];
	char secret[1300];
#ifdef _WIN32
	char stream[1500];
#endif

	/* Keep the progress lines if the test is killed on a timeout. */
	setvbuf(stdout, NULL, _IONBF, 0);
	if (!get_cwd(cwd, sizeof cwd)) {
		fprintf(stderr, "no working directory; skipping\n");
		return EXIT_SUCCESS;
	}
	snprintf(root, sizeof root, "%s/" TEST_DIR, cwd);
	snprintf(docroot, sizeof docroot, "%s/docroot", root);
	snprintf(sub, sizeof sub, "%s/sub", docroot);
	snprintf(inside, sizeof inside, "%s/inside.txt", docroot);
	snprintf(secret, sizeof secret, "%s/secret.txt", root);
	make_dir(root);
	make_dir(docroot);
	make_dir(sub);
#ifdef _WIN32
	/* A named stream of the file in the document root. */
	snprintf(stream, sizeof stream, "%s:stream", inside);
#endif
	if (write_file(inside, INSIDE_TEXT) != 0 ||
		write_file(secret, SECRET_TEXT) != 0
#ifdef _WIN32
		|| write_file(stream, SECRET_TEXT) != 0
#endif
	) {
		fprintf(stderr, "cannot create the test files\n");
		return EXIT_FAILURE;
	}

	rc = UpnpInit2(NULL, 0);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr,
			"UpnpInit2 failed (%d); skipping (no network?)\n",
			rc);
		err = -1;
		goto cleanup;
	}
	ip = UpnpGetServerIpAddress();
	port = UpnpGetServerPort();
	if (!ip || port == 0 || UpnpSetWebServerRootDir(docroot) != 0) {
		fprintf(stderr, "no server address or root; skipping\n");
		err = -1;
		goto finish;
	}

	/* The server must serve the file inside the root, or the checks
	 * below mean nothing. */
	rc = get_path(ip, port, "/inside.txt", &leaked);
	printf("GET %-32s -> %d\n", "/inside.txt", rc);
	if (rc != 200) {
		fprintf(stderr,
			"FAIL: the file inside the root is not served\n");
		err = 1;
		goto finish;
	}

	/* "Go up one directory" written with a forward slash is resolved by
	 * remove_dots() on every system. */
	err |= check_blocked(ip, port, "/..%2fsecret.txt");
	err |= check_blocked(ip, port, "/../secret.txt");
	/* The same with a backslash, in the ways that decode to one. */
	err |= check_blocked(ip, port, "/..%5csecret.txt");
	err |= check_blocked(ip, port, "/%2e%2e%5csecret.txt");
	err |= check_blocked(ip, port, "/sub%5c..%5c..%5csecret.txt");
	err |= check_blocked(ip, port, "/sub/..%5c..%5csecret.txt");
#ifdef _WIN32
	/* Alternate data streams of a file in the root. */
	err |= check_blocked(ip, port, "/inside.txt:stream");
	err |= check_blocked(ip, port, "/inside.txt::$DATA");
#endif

finish:
	UpnpFinish();
cleanup:
	remove_file(inside);
	remove_file(secret);
	remove_dir(sub);
	remove_dir(docroot);
	remove_dir(root);
	if (err > 0) {
		return EXIT_FAILURE;
	}
	if (err == 0)
		puts("test_webserver_backslash: PASS");

	return EXIT_SUCCESS;
}
