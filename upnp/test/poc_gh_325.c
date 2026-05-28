/* poc_gh_325.c
 *
 * Regression test for issue #325: crash in alias_release() when UpnpFinish()
 * destroys gWebMutex while a thread pool thread still holds or waits on it.
 *
 * Sends repeated HTTP GET requests to the alias URL while calling UpnpFinish().
 * Each request dispatches a thread pool thread through web_server_callback()
 * which calls alias_grab() / alias_release(), exercising gWebMutex.
 *
 * Pre-fix: web_server_destroy() fires before ThreadPoolShutdown() drains those
 *          handler threads → assertion failure / mutex use-after-destroy.
 * Post-fix: ThreadPoolShutdown() completes first → all handler threads have
 *           exited → web_server_destroy() fires safely → no crash.
 *
 * Run with TSan (-fsanitize=thread) for reliable race detection.
 * regression: issue #325
 */

#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32

	#include "upnp.h"

	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <pthread.h>
	#include <stddef.h>
	#include <string.h>
	#include <sys/socket.h>
	#include <unistd.h>

/* regression: issue #325 -- test hook exported from libupnp */
extern int web_server_ut_set_alias(
	const char *name, const char *content, size_t len);

	#ifndef MSG_NOSIGNAL
		#define MSG_NOSIGNAL 0
	#endif

static _Atomic int g_keep_requesting = 1;
static unsigned short g_port;

static void send_one_request(void)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		return;

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(g_port);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

	if (connect(sock, (struct sockaddr *)&addr, sizeof addr) == 0) {
		const char *req = "GET /desc.xml HTTP/1.1\r\n"
				  "Host: localhost\r\n"
				  "Connection: close\r\n\r\n";
		send(sock, req, strlen(req), MSG_NOSIGNAL);
		char buf[256];
		while (recv(sock, buf, sizeof buf, 0) > 0)
			;
	}
	close(sock);
}

static void *request_thread(void *arg)
{
	(void)arg;
	while (g_keep_requesting)
		send_one_request();
	return NULL;
}

int main(void)
{
	int rc;
	pthread_t thr;

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

	g_port = UpnpGetServerPort();
	if (g_port == 0) {
		fprintf(stderr, "UpnpGetServerPort returned 0; skipping\n");
		UpnpFinish();
		return EXIT_SUCCESS;
	}

	if (pthread_create(&thr, NULL, request_thread, NULL) != 0) {
		fprintf(stderr, "pthread_create failed\n");
		UpnpFinish();
		return EXIT_FAILURE;
	}

	/* Let HTTP requests flow long enough to have handler threads active
	 * in alias_grab() / alias_release() when UpnpFinish() fires. */
	usleep(10000); /* 10 ms */

	/* Pre-fix: web_server_destroy() fires before ThreadPoolShutdown()
	 * drains the handler threads → crash.
	 * Post-fix: ThreadPoolShutdown() completes first → safe shutdown. */
	UpnpFinish();

	g_keep_requesting = 0;
	pthread_join(thr, NULL);

	puts("PASS: no crash detected.");
	return EXIT_SUCCESS;
}

#else /* _WIN32 */

int main(void)
{
	puts("SKIP: test uses POSIX sockets (not available on Windows).");
	return EXIT_SUCCESS;
}

#endif /* _WIN32 */
