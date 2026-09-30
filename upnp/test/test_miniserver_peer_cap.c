/* test_miniserver_peer_cap.c
 *
 * Regression test: one peer must not be able to take all the workers of the
 * miniserver.
 *
 * Every accepted connection is served by one worker of a small thread pool
 * (12 threads, one of them the accept loop), and nothing limited how many
 * connections a single peer could hold. A peer that opened more connections
 * than there are workers kept all of them busy and no other client was
 * served.
 *
 * The miniserver now serves at most MINISERVER_MAX_CONNECTIONS_PER_PEER
 * connections at a time for one address and closes the others at once. The
 * test opens more connections than that, all silent, from 127.0.0.1, and
 * checks that exactly the excess ones are closed and that a client at the
 * address of the server's interface is served straight away. It then checks
 * that the slots are given back when the connections end.
 *
 * The silent connections are made from 127.0.0.1 to the address of the
 * server's interface, which is also the source address of the client that
 * must be served, so they are two different peers. Choosing the local source
 * address with bind() is only relied on for Linux.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__linux__)

	#include "config.h"
	#include "upnp.h"

	#include <arpa/inet.h>
	#include <errno.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <time.h>
	#include <unistd.h>

/* Test hook exported from libupnp. */
extern int web_server_ut_set_alias(
	const char *name, const char *content, size_t len);

	#define EXTRA_CONNECTIONS 8
	#define NUM_SILENT \
		(MINISERVER_MAX_CONNECTIONS_PER_PEER + EXTRA_CONNECTIONS)

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
 * nothing useful arrives within timeout_sec. */
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

/* Returns 1 when the server has closed the connection, 0 when it is open. */
static int closed_by_server(int fd)
{
	char c;
	ssize_t n = recv(fd, &c, 1, MSG_DONTWAIT);

	if (n == 0)
		return 1;
	if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
		return 1;

	return 0;
}

int main(void)
{
	int rc;
	int i;
	int err = 0;
	int status;
	int fd;
	int closed;
	int served_again = 0;
	int silent[NUM_SILENT];
	const char *ip;
	unsigned short port;

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
		fprintf(stderr, "no server address or alias; skipping\n");
		UpnpFinish();
		return EXIT_SUCCESS;
	}

	/* One peer opens more silent connections than it may hold. */
	for (i = 0; i < NUM_SILENT; i++)
		silent[i] = connect_from("127.0.0.1", ip, port);
	if (silent[0] < 0) {
		fprintf(stderr, "cannot connect from 127.0.0.1; skipping\n");
		UpnpFinish();
		return EXIT_SUCCESS;
	}
	sleep(1); /* The accept loop takes them one by one. */
	closed = 0;
	for (i = 0; i < NUM_SILENT; i++)
		if (silent[i] >= 0 && closed_by_server(silent[i]))
			closed++;
	printf("%d silent connections from one peer: %d closed by the "
	       "server (expected %d)\n",
		NUM_SILENT,
		closed,
		EXTRA_CONNECTIONS);
	if (closed != EXTRA_CONNECTIONS) {
		fprintf(stderr,
			"FAIL: the peer was allowed to hold %d connections "
			"(limit %d)\n",
			NUM_SILENT - closed,
			MINISERVER_MAX_CONNECTIONS_PER_PEER);
		err = 1;
	}

	/* Another peer is served straight away. */
	fd = connect_from(NULL, ip, port);
	status = fd < 0 ? -1 : get_status(fd, ip, port, 3);
	printf("request from another peer: %d\n", status);
	if (status != 200) {
		fprintf(stderr,
			"FAIL: another peer was not served while the first "
			"one held its connections\n");
		err = 1;
	}
	if (fd >= 0)
		close(fd);

	/* The slots are given back when the connections end. */
	for (i = 0; i < NUM_SILENT; i++)
		if (silent[i] >= 0)
			close(silent[i]);
	for (i = 0; i < 15 && !served_again; i++) {
		usleep(200 * 1000);
		fd = connect_from("127.0.0.1", ip, port);
		if (fd < 0)
			continue;
		served_again = get_status(fd, ip, port, 2) == 200;
		close(fd);
	}
	printf("same peer after closing its connections: %s\n",
		served_again ? "served" : "NOT served");
	if (!served_again) {
		fprintf(stderr, "FAIL: the slots of the peer were not freed\n");
		err = 1;
	}

	web_server_ut_set_alias(NULL, NULL, 0);
	UpnpFinish();
	if (!err)
		puts("test_miniserver_peer_cap: PASS");

	return err ? EXIT_FAILURE : EXIT_SUCCESS;
}

#else /* !__linux__ */

int main(void)
{
	puts("SKIP: test needs to choose its source address (Linux only).");
	return EXIT_SUCCESS;
}

#endif /* __linux__ */
