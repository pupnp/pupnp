/* test_miniserver_first_byte.c
 *
 * Regression test: connections that stay silent must not starve the
 * miniserver.
 *
 * Every accepted connection is served by one worker of a small thread pool
 * (12 threads, one of them the accept loop). A worker waited up to 30
 * seconds for the request of a connection that sent nothing, so a peer that
 * merely opened a dozen connections kept every worker busy and nobody else
 * was served until the timeouts expired.
 *
 * A connection must now send its first byte within a few seconds or it is
 * dropped. The test opens more silent connections than there are workers,
 * then expects a request from another address to be served in a short time.
 * It also checks that a client that is a little slow to send its request is
 * still served.
 *
 * The silent connections come from several addresses of the 127.0.0.0/8
 * block, a few from each, so that no single peer holds more than a limit on
 * the connections of one peer would allow. They are made to the address of
 * the server's interface, which is also the source address of the client
 * that must be served. Connecting from a chosen local source address is done
 * with bind(), which is portable enough for Linux only.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__linux__)

	#include "upnp.h"

	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <time.h>
	#include <unistd.h>

/* Test hook exported from libupnp. */
extern int web_server_ut_set_alias(
	const char *name, const char *content, size_t len);

	#define NUM_PEERS 4
	#define SILENT_PER_PEER 4
	/* More than the workers of the pool. */
	#define NUM_SILENT (NUM_PEERS * SILENT_PER_PEER)
	#define SERVED_WITHIN_SEC 10

static double now_sec(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

/* Connects to dst_ip:port, from the source address src_ip when it is not
 * NULL. */
static int connect_from(
	const char *src_ip, const char *dst_ip, unsigned short port)
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in src;
	struct sockaddr_in dst;

	if (fd < 0)
		return -1;
	memset(&src, 0, sizeof src);
	src.sin_family = AF_INET;
	memset(&dst, 0, sizeof dst);
	dst.sin_family = AF_INET;
	dst.sin_port = htons(port);
	inet_pton(AF_INET, dst_ip, &dst.sin_addr);
	if (src_ip) {
		inet_pton(AF_INET, src_ip, &src.sin_addr);
		if (bind(fd, (struct sockaddr *)&src, sizeof src) != 0) {
			close(fd);
			return -1;
		}
	}
	if (connect(fd, (struct sockaddr *)&dst, sizeof dst) != 0) {
		close(fd);
		return -1;
	}

	return fd;
}

/* Sends a GET for the alias on fd and returns the status code, or -1 when
 * nothing arrives within timeout_sec. */
static int get_status(
	int fd, const char *host_ip, unsigned short port, int timeout_sec)
{
	char req[256];
	char buf[512];
	struct timeval tv;
	ssize_t n;
	int status = -1;

	tv.tv_sec = timeout_sec;
	tv.tv_usec = 0;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
	snprintf(req,
		sizeof req,
		"GET /desc.xml HTTP/1.1\r\n"
		"Host: %s:%u\r\n"
		"Connection: close\r\n\r\n",
		host_ip,
		port);
	if (send(fd, req, strlen(req), MSG_NOSIGNAL) < 0)
		return -1;
	n = recv(fd, buf, sizeof buf - 1, 0);
	if (n > 0) {
		buf[n] = '\0';
		sscanf(buf, "HTTP/%*d.%*d %d", &status);
	}

	return status;
}

int main(void)
{
	int rc;
	int i;
	int err = 0;
	int status;
	int fd;
	int silent[NUM_SILENT];
	const char *ip;
	unsigned short port;
	double t0;
	double elapsed;

	/* Keep the progress lines if the test is killed on a timeout. */
	setvbuf(stdout, NULL, _IOLBF, 0);
	rc = UpnpInit2(NULL, 0);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr,
			"UpnpInit2 failed (%d); skipping (no network?)\n",
			rc);
		return EXIT_SUCCESS;
	}
	ip = UpnpGetServerIpAddress();
	port = UpnpGetServerPort();
	if (!ip || port == 0 ||
		web_server_ut_set_alias("/desc.xml", "<root/>", 7) !=
			UPNP_E_SUCCESS) {
		fprintf(stderr, "no server port or alias; skipping\n");
		UpnpFinish();
		return EXIT_SUCCESS;
	}

	/* A client that is a little slow to send its request is served. */
	fd = connect_from(NULL, ip, port);
	if (fd < 0) {
		fprintf(stderr,
			"cannot connect to %s:%u; skipping\n",
			ip,
			port);
		UpnpFinish();
		return EXIT_SUCCESS;
	}
	sleep(2);
	status = get_status(fd, ip, port, SERVED_WITHIN_SEC);
	close(fd);
	printf("slow client (2 s before its request): %d\n", status);
	if (status != 200) {
		fprintf(stderr,
			"FAIL: a slightly slow client was not served\n");
		err = 1;
	}

	/* Silent connections from other addresses must not starve it. */
	for (i = 0; i < NUM_SILENT; i++) {
		char src[32];

		snprintf(
			src, sizeof src, "127.0.0.%d", i / SILENT_PER_PEER + 1);
		silent[i] = connect_from(src, ip, port);
	}
	if (silent[0] < 0) {
		fprintf(stderr, "cannot connect from 127.0.0.1; skipping\n");
		UpnpFinish();
		return err ? EXIT_FAILURE : EXIT_SUCCESS;
	}
	fd = connect_from(NULL, ip, port);
	t0 = now_sec();
	status = fd < 0 ? -1 : get_status(fd, ip, port, SERVED_WITHIN_SEC);
	elapsed = now_sec() - t0;
	printf("request behind %d silent connections: %d after %.1f s\n",
		NUM_SILENT,
		status,
		elapsed);
	if (status != 200) {
		fprintf(stderr,
			"FAIL: not served within %d s while %d silent "
			"connections were open\n",
			SERVED_WITHIN_SEC,
			NUM_SILENT);
		err = 1;
	}
	if (fd >= 0)
		close(fd);
	for (i = 0; i < NUM_SILENT; i++)
		if (silent[i] >= 0)
			close(silent[i]);

	web_server_ut_set_alias(NULL, NULL, 0);
	UpnpFinish();
	if (!err)
		puts("test_miniserver_first_byte: PASS");

	return err ? EXIT_FAILURE : EXIT_SUCCESS;
}

#else /* !__linux__ */

int main(void)
{
	puts("SKIP: test needs the 127/8 loopback block (Linux only).");
	return EXIT_SUCCESS;
}

#endif /* __linux__ */
