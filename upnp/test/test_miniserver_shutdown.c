/* test_miniserver_shutdown.c
 *
 * Regression test: UpnpFinish() must not hang when connections are being
 * served while the miniserver stops.
 *
 * When the miniserver stops it shuts down the connections that are still
 * open and used to destroy the mutex that protects its list of active
 * connections. The workers serving those connections wake up, take the
 * connection off the list and therefore lock that mutex: one that got there
 * after it had been destroyed blocked forever, and UpnpFinish() waited for it
 * in ThreadPoolShutdown().
 *
 * The test opens several connections that send nothing, so that workers are
 * blocked reading them, and calls UpnpFinish() without closing them. It does
 * this a number of times, since the outcome depends on timing. A watchdog
 * turns a hang into a failure.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32

	#include "upnp.h"

	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <signal.h>
	#include <sys/socket.h>
	#include <unistd.h>

	#define CYCLES 10
	#define IDLE_CONNECTIONS 60
	#define WATCHDOG_SEC 60

static void on_timeout(int sig)
{
	static const char msg[] =
		"FAIL: UpnpFinish() did not return (shutdown hang)\n";

	(void)sig;
	if (write(STDERR_FILENO, msg, sizeof msg - 1) < 0) {
		/* Nothing more to do. */
	}
	_exit(EXIT_FAILURE);
}

static int connect_to(const char *ip, unsigned short port)
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in dst;

	if (fd < 0)
		return -1;
	memset(&dst, 0, sizeof dst);
	dst.sin_family = AF_INET;
	dst.sin_port = htons(port);
	inet_pton(AF_INET, ip, &dst.sin_addr);
	if (connect(fd, (struct sockaddr *)&dst, sizeof dst) != 0) {
		close(fd);
		return -1;
	}

	return fd;
}

int main(void)
{
	int cycle;
	int i;
	int fds[IDLE_CONNECTIONS];
	const char *ip;
	unsigned short port;

	setvbuf(stdout, NULL, _IOLBF, 0);
	signal(SIGALRM, on_timeout);
	alarm(WATCHDOG_SEC);
	for (cycle = 0; cycle < CYCLES; cycle++) {
		if (UpnpInit2(NULL, 0) != UPNP_E_SUCCESS) {
			fprintf(stderr,
				"UpnpInit2 failed; skipping (no network?)\n");
			return EXIT_SUCCESS;
		}
		ip = UpnpGetServerIpAddress();
		port = UpnpGetServerPort();
		if (!ip || port == 0) {
			fprintf(stderr, "no server address; skipping\n");
			UpnpFinish();
			return EXIT_SUCCESS;
		}
		for (i = 0; i < IDLE_CONNECTIONS; i++)
			fds[i] = connect_to(ip, port);
		/* Let the workers pick the connections up and block on them. */
		usleep(200 * 1000);
		UpnpFinish();
		for (i = 0; i < IDLE_CONNECTIONS; i++)
			if (fds[i] >= 0)
				close(fds[i]);
	}
	alarm(0);
	printf("test_miniserver_shutdown: PASS (%d cycles)\n", CYCLES);

	return EXIT_SUCCESS;
}

#else /* _WIN32 */

int main(void)
{
	puts("SKIP: test uses POSIX sockets and alarm() (not on Windows).");
	return EXIT_SUCCESS;
}

#endif /* !_WIN32 */
