/* test_webserver_settings_race.c
 *
 * Regression test: the web server settings that the API changes must not be
 * freed while a request reads them.
 *
 * UpnpAddVirtualDir(), UpnpRemoveVirtualDir(), UpnpSetWebServerRootDir() and
 * UpnpSetWebServerCorsString() changed the virtual directory list, the
 * document root and the CORS string without a lock, while the miniserver
 * threads read them for every GET: isFileInVirtualDir() walks the list, and
 * process_request() copies the document root and keeps a pointer to the CORS
 * string until the headers are made. A request served while the application
 * changed them read freed memory: AddressSanitizer reports a
 * heap-use-after-free, ThreadSanitizer a data race.
 *
 * Client threads request a file of the document root while the main thread
 * adds and removes virtual directories and switches between two document
 * roots, which both have the file, and between two CORS strings. Every answer
 * must be 200, with one of the two bodies and one of the two CORS values.
 * Without the fix the test fails under the sanitizers and can crash or get a
 * 404 without them.
 *
 * The stack runs on the loopback interface, so no network is needed.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32

	#include "upnp.h"

	#include <arpa/inet.h>
	#include <ifaddrs.h>
	#include <net/if.h>
	#include <netinet/in.h>
	#include <pthread.h>
	#include <signal.h>
	#include <sys/socket.h>
	#include <unistd.h>

	#ifndef MSG_NOSIGNAL
		#define MSG_NOSIGNAL 0
	#endif

	#define NUM_CLIENTS 3
	#define NUM_REQUESTS 200
	#define NUM_DIRS 16
	#define CORS_A "http://a.example"
	#define CORS_B "http://b.example"

static const char *gIp;
static unsigned short gPort;
static pthread_mutex_t gMutex = PTHREAD_MUTEX_INITIALIZER;
static int gClientsDone;
static int gFailed;

/* Copies the name of an up IPv4 loopback interface. */
static int find_loopback(char *name, size_t name_size)
{
	struct ifaddrs *ifap;
	struct ifaddrs *ifa;
	int found = 0;

	if (getifaddrs(&ifap) != 0)
		return 0;
	for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
		if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET ||
			!(ifa->ifa_flags & IFF_LOOPBACK) ||
			!(ifa->ifa_flags & IFF_UP))
			continue;
		snprintf(name, name_size, "%s", ifa->ifa_name);
		found = 1;
		break;
	}
	freeifaddrs(ifap);

	return found;
}

/* Sends one GET of /hello.txt and stores the response, NUL terminated.
 * Returns the number of bytes received, or -1 if the connection failed. */
static int get_hello(char *buf, size_t size)
{
	int sock;
	struct sockaddr_in addr;
	char req[256];
	ssize_t n;
	size_t total = 0;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		return -1;
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(gPort);
	inet_pton(AF_INET, gIp, &addr.sin_addr);
	if (connect(sock, (struct sockaddr *)&addr, sizeof addr) != 0) {
		close(sock);
		return -1;
	}
	snprintf(req,
		sizeof req,
		"GET /hello.txt HTTP/1.1\r\n"
		"Host: %s:%u\r\n"
		"Connection: close\r\n\r\n",
		gIp,
		gPort);
	send(sock, req, strlen(req), MSG_NOSIGNAL);
	while (total < size - 1 &&
		(n = recv(sock, buf + total, size - 1 - total, 0)) > 0) {
		total += (size_t)n;
	}
	buf[total] = '\0';
	close(sock);

	return (int)total;
}

/* Returns 0 for a 200 with body "a" or "b" and one of the CORS values. */
static int check_response(const char *buf)
{
	int status = -1;
	const char *body;

	sscanf(buf, "HTTP/%*d.%*d %d", &status);
	if (status != 200) {
		fprintf(stderr, "FAIL: status %d\n", status);
		return 1;
	}
	body = strstr(buf, "\r\n\r\n");
	if (!body ||
		(strcmp(body + 4, "a") != 0 && strcmp(body + 4, "b") != 0)) {
		fprintf(stderr, "FAIL: unexpected body\n");
		return 1;
	}
	if (!strstr(buf, "Access-Control-Allow-Origin: " CORS_A "\r\n") &&
		!strstr(buf, "Access-Control-Allow-Origin: " CORS_B "\r\n")) {
		fprintf(stderr, "FAIL: unexpected CORS header\n");
		return 1;
	}

	return 0;
}

static void *client(void *arg)
{
	char buf[2048];
	int i;
	int n;
	int retries = 0;
	int failed = 0;

	(void)arg;
	for (i = 0; i < NUM_REQUESTS && !failed; i++) {
		n = get_hello(buf, sizeof buf);
		if (n <= 0) {
			/* The miniserver closes a connection beyond its per
			 * peer limit without answering: try again. */
			if (++retries > 10 * NUM_REQUESTS) {
				fprintf(stderr, "FAIL: no answer\n");
				failed = 1;
			}
			i--;
			continue;
		}
		failed = check_response(buf);
	}
	pthread_mutex_lock(&gMutex);
	gClientsDone++;
	gFailed |= failed;
	pthread_mutex_unlock(&gMutex);

	return NULL;
}

static int clients_done(void)
{
	int done;

	pthread_mutex_lock(&gMutex);
	done = gClientsDone == NUM_CLIENTS;
	pthread_mutex_unlock(&gMutex);

	return done;
}

static int get_info(const char *filename,
	UpnpFileInfo *info,
	const void *cookie,
	const void **request_cookie)
{
	(void)filename;
	(void)info;
	(void)cookie;
	(void)request_cookie;

	return -1;
}

static int make_root(char *root, const char *body)
{
	char file[64];
	FILE *f;

	if (!mkdtemp(root)) {
		perror("mkdtemp");
		return 1;
	}
	snprintf(file, sizeof file, "%s/hello.txt", root);
	f = fopen(file, "w");
	if (!f) {
		perror("fopen");
		return 1;
	}
	fputs(body, f);
	fclose(f);

	return 0;
}

static void remove_root(const char *root)
{
	char file[64];

	snprintf(file, sizeof file, "%s/hello.txt", root);
	unlink(file);
	rmdir(root);
}

int main(void)
{
	char name[IF_NAMESIZE + 1];
	char root_a[] = "/tmp/upnp_settings_race_XXXXXX";
	char root_b[] = "/tmp/upnp_settings_race_XXXXXX";
	char dir[32];
	pthread_t threads[NUM_CLIENTS];
	int started = 0;
	int err = 0;
	int rounds = 0;
	int i;
	int rc;

	/* A connection beyond the per peer limit is closed unanswered. */
	signal(SIGPIPE, SIG_IGN);
	if (!find_loopback(name, sizeof name)) {
		puts("SKIP: no IPv4 loopback interface is up");
		return EXIT_SUCCESS;
	}
	rc = UpnpInit2(name, 0);
	if (rc != UPNP_E_SUCCESS) {
		fprintf(stderr, "UpnpInit2(\"%s\") failed (%d)\n", name, rc);
		return EXIT_FAILURE;
	}
	gIp = UpnpGetServerIpAddress();
	gPort = UpnpGetServerPort();
	if (make_root(root_a, "a") || make_root(root_b, "b") ||
		UpnpVirtualDir_set_GetInfoCallback(get_info) !=
			UPNP_E_SUCCESS ||
		UpnpSetWebServerRootDir(root_a) != UPNP_E_SUCCESS ||
		UpnpSetWebServerCorsString(CORS_A) != UPNP_E_SUCCESS) {
		fprintf(stderr, "setup failed\n");
		err = 1;
	}
	for (i = 0; !err && i < NUM_CLIENTS; i++) {
		if (pthread_create(&threads[i], NULL, client, NULL) != 0) {
			fprintf(stderr, "pthread_create failed\n");
			err = 1;
			break;
		}
		started++;
	}
	while (!err && !clients_done()) {
		for (i = 0; i < NUM_DIRS; i++) {
			snprintf(dir, sizeof dir, "/settings_race_%d", i);
			UpnpAddVirtualDir(dir, NULL, NULL);
		}
		UpnpSetWebServerRootDir(root_b);
		UpnpSetWebServerCorsString(CORS_B);
		UpnpSetWebServerRootDir(root_a);
		UpnpSetWebServerCorsString(CORS_A);
		for (i = 0; i < NUM_DIRS; i++) {
			snprintf(dir, sizeof dir, "/settings_race_%d", i);
			UpnpRemoveVirtualDir(dir);
		}
		rounds++;
	}
	for (i = 0; i < started; i++)
		pthread_join(threads[i], NULL);
	err |= gFailed;
	printf("%d requests while the settings changed %d times\n",
		NUM_CLIENTS * NUM_REQUESTS,
		rounds);

	UpnpFinish();
	remove_root(root_a);
	remove_root(root_b);
	if (!err)
		puts("test_webserver_settings_race: PASS");

	return err ? EXIT_FAILURE : EXIT_SUCCESS;
}

#else /* _WIN32 */

int main(void)
{
	puts("SKIP: test uses POSIX sockets (not available on Windows).");
	return EXIT_SUCCESS;
}

#endif /* _WIN32 */
